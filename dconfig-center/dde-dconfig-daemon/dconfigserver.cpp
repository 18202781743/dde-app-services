// SPDX-FileCopyrightText: 2021 - 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigserver.h"
#include "dconfigresource.h"
#include "dconfigconn.h"
#include "dconfigrefmanager.h"
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QCoreApplication>
#include <QDebug>
#include <QLoggingCategory>
#include <QDir>
#include <QFile>

#include "configmanager_adaptor.h"

#define DSG_CONFIG "org.desktopspec.ConfigManager"

#ifndef QT_DEBUG
Q_LOGGING_CATEGORY(cfLog, "dsg.config", QtInfoMsg);
#else
Q_LOGGING_CATEGORY(cfLog, "dsg.config");
#endif

__attribute__((constructor)) // 在库被加载时就执行此函数
static void registerMetaType ()
{
    qRegisterMetaType<ConnServiceName>("ConnServiceName");
    qRegisterMetaType<ConnKey>("ConnKey");
}

DSGConfigServer::DSGConfigServer(QObject *parent)
    :QObject (parent),
      m_watcher(nullptr),
      m_refManager(new RefManager(this))
    , m_syncRequestCache(new ConfigSyncRequestCache(this))
{
    connect(this, &DSGConfigServer::releaseResource, this, &DSGConfigServer::onReleaseResource);
    connect(m_refManager, &RefManager::releaseResource, this, &DSGConfigServer::releaseResource);
    connect(this, &DSGConfigServer::tryExit, this, &DSGConfigServer::onTryExit);
    connect(m_syncRequestCache, &ConfigSyncRequestCache::syncConfigRequest, this, &DSGConfigServer::doSyncConfigCache);
}

DSGConfigServer::~DSGConfigServer()
{
    qInfo() << "Destory DSGConfigServer and try to release resources.";
    exit();
}

void DSGConfigServer::exit()
{
    m_refManager->destroy();
    qDeleteAll(m_resources);
    m_resources.clear();
    m_syncRequestCache->clear();
}

/*
    \breaf 注册服务到dbus上
    \return 是否注册成功
*/
bool DSGConfigServer::registerService()
{
    (void) new DSGConfigAdaptor(this);

    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.registerService(DSG_CONFIG)) {
        QString errorMsg = bus.lastError().message();
        if (errorMsg.isEmpty())
            errorMsg = "maybe it's running";

        qWarning() << QString("Can't register the %1 service, %2.").arg(DSG_CONFIG).arg(errorMsg);
        return false;
    }
    if (!bus.registerObject("/", this)) {
        qWarning() << QString("Can't register to the D-Bus object.");
        return false;
    }
    return true;
}

void DSGConfigServer::initialize()
{
    // Initialize file signatures to avoid unnecessary updates on first reload
    qCInfo(cfLog()) << "Initializing file signatures on service startup";
    m_fileSignatures = allConfigureFileSignatures(m_localPrefix);
    qCInfo(cfLog()) << "Initialized file signatures completed, size: " << m_fileSignatures.size();
}

/*!
 \brief 获得指定连接key值的连接对象
 \a key 连接对象的唯一ID
 \return
 */
DSGConfigResource *DSGConfigServer::resourceObject(const GenericResourceKey &key) const
{
    return m_resources.value(key);
}

/*!
 \brief 设置延迟释放的时间
 \a ms 延迟释放时间,单位为毫秒
 */
void DSGConfigServer::setDelayReleaseTime(const int ms)
{
    if (ms < 0) {
        QString errorMsg = QString("Negative values are not supported for delayed release time.");
        if (calledFromDBus())
            sendErrorReply(QDBusError::InvalidArgs, errorMsg);
        qCWarning(cfLog()) << qPrintable(errorMsg);
        return;
    }
    m_refManager->setDelayReleaseTime(ms);
}

int DSGConfigServer::delayReleaseTime() const
{
    return m_refManager->delayReleaseTime();
}

void DSGConfigServer::enableVerboseLogging()
{
    QByteArrayList rules{QString("%1.debug=true").arg(cfLog().categoryName()).toLocal8Bit()};
    rules << "dtk.dsg.config.debug=true";
    setLogRules(rules.join(';'));
}

void DSGConfigServer::disableVerboseLogging()
{
    setLogRules("");
}

void DSGConfigServer::setLogRules(const QString &rules)
{
    QByteArrayList result;
    for (auto item : rules.split(";")) {
        result << item.toLocal8Bit();
    }
    QLoggingCategory::setFilterRules(result.join('\n'));
    qCInfo(cfLog(), "Set log filter rules to:\"%s\"", qPrintable(rules));
}

/*!
 \brief 删除指定用户的所有配置数据
 删除指定用户的内存连接、缓存以及文件系统中的配置目录。
 权限控制完全通过D-Bus安全策略实现，无需额外验证。
 \a uid 要删除配置数据的用户ID
 */
void DSGConfigServer::removeUserData(const uint &uid)
{
    qCInfo(cfLog()) << QString("Starting to remove user data for UID %1").arg(uid);

    // 收集要删除的连接
    QList<ConnKey> connectionsToRemove;
    for (auto iter = m_resources.begin(); iter != m_resources.end(); ++iter) {
        auto resource = iter.value();
        if (!resource)
            continue;

        // 获取该用户在此资源中的所有连接
        const QList<ConnKey> userConnections = resource->getConnectionsByUid(uid);
        connectionsToRemove.append(userConnections);

        for (const ConnKey &connKey : userConnections) {
            qCDebug(cfLog()) << QString("Found connection to remove: %1").arg(connKey);
        }
    }

    // 逐个删除连接和相关数据
    int removedCount = 0;
    for (const ConnKey &connKey : connectionsToRemove) {
        const GenericResourceKey &resourceKey = getGenericResourceKey(connKey);
        auto resource = m_resources.value(resourceKey);
        if (resource) {
            // 删除连接，这会自动保存并删除相关的缓存和配置文件
            resource->removeConn(connKey);
            removedCount++;

            qCInfo(cfLog()) << QString("Removed connection: %1").arg(connKey);

            // 如果资源没有更多连接，清理资源
            if (resource->isEmptyConn()) {
                qCInfo(cfLog()) << QString("Removing empty resource: %1").arg(resourceKey);
                m_resources.remove(resourceKey);
                resource->deleteLater();
            }
        }
        }

    // 删除文件系统中的用户配置目录
    const QString userConfigBasePath = QString("%1/%2").arg(m_localPrefix).arg(configPrefixPath());
    if (!userConfigBasePath.isEmpty()) {
        const QString userCacheDir = QString("%1/%2").arg(userConfigBasePath).arg(uid);
        QDir cacheDir(userCacheDir);
        if (cacheDir.exists()) {
            if (cacheDir.removeRecursively()) {
                qCInfo(cfLog()) << QString("Removed user cache directory: %1").arg(userCacheDir);
            } else {
                qCWarning(cfLog()) << QString("Failed to remove user cache directory: %1").arg(userCacheDir);
            }
        }
    }

    qCInfo(cfLog()) << QString("Successfully removed %1 connections for user UID %2").arg(removedCount).arg(uid);
}

void DSGConfigServer::setLocalPrefix(const QString &localPrefix)
{
    m_localPrefix = localPrefix;
}

void DSGConfigServer::setEnableExit(const bool enable)
{
    m_enableExit = enable;
}

int DSGConfigServer::resourceSize() const
{
    return m_resources.size();
}

/*!
 \brief 响应请求配置文件管理连接
 \a 应用程序的唯一ID
 \a 配置文件名
 \a 配置文件子目录
 \return
 */
QDBusObjectPath DSGConfigServer::acquireManager(const QString &appid, const QString &name, const QString &subpath)
{
    const auto &service = calledFromDBus() ? message().service() : "test.service";
    const uint &uid = calledFromDBus() ? connection().interface()->serviceUid(service).value() : TestUid;
    return acquireManagerV2(uid, appid, name, subpath);
}

/*!
 \brief 响应请求配置文件管理连接
 \a 用户的唯一ID
 \a 应用程序的唯一ID
 \a 配置文件名
 \a 配置文件子目录
 \return
 */
QDBusObjectPath DSGConfigServer::acquireManagerV2(const uint &uid, const QString &appid, const QString &name, const QString &subpath)
{
    struct passwd *pw = getpwuid(uid);
    if (!pw) {
        QString errorMsg = QString("User with UID %1 does not exist.").arg(uid);
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, errorMsg);
        qWarning() << qPrintable(errorMsg);
        return QDBusObjectPath();
    }

    const auto &service = calledFromDBus() ? message().service() : "test.service";
    qCDebug(cfLog, "AcquireManager service:%s, uid:%d, appid:%s", qPrintable(service), uid, qPrintable(appid));
    const QString &innerAppid = outerAppidToInner(appid);
    const GenericResourceKey &genericResourceKey = getGenericResourceKey(name, subpath);
    DSGConfigResource *resource = resourceObject(genericResourceKey);
    std::unique_ptr<DSGConfigResource> resourceHolder;
    if (!resource) {
        resource = new DSGConfigResource(name, subpath, m_localPrefix);
        resource->setSyncRequestCache(m_syncRequestCache);
        resourceHolder.reset(resource);
    }
    bool loadStatus = resource->load(innerAppid);
    if (!loadStatus) {
        //error
        QString errorMsg = QString("Can't load resource: %1, for the appid:[%2].").arg(genericResourceKey).arg(appid);
        if (calledFromDBus())
            sendErrorReply(QDBusError::Failed, errorMsg);

        qWarning() << qPrintable(errorMsg);
        return QDBusObjectPath();
    }

    auto conn = resource->getConn(innerAppid, uid);
    if (!conn) {
        conn = resource->createConn(innerAppid, uid);
        if (!conn) {
            QString errorMsg = QString("Can't register Connection object:[%1], for the appid:[%2].").arg(genericResourceKey).arg(appid);
            if (calledFromDBus())
                sendErrorReply(QDBusError::Failed, errorMsg);

            qWarning() << qPrintable(errorMsg);
            return QDBusObjectPath();
        }
        qCInfo(cfLog, "Created connection:%s", qPrintable(conn->path()));
    } else {
        qCInfo(cfLog, "Reuse connection:%s", qPrintable(conn->path()));
    }

    if (resourceHolder) {
        m_resources.insert(genericResourceKey, resourceHolder.release());
        QObject::connect(resource, &DSGConfigResource::releaseConn, this, &DSGConfigServer::onReleaseChanged);
    }

    addConnWatchedService(service);
    m_refManager->refResource(service, conn->key());

    return QDBusObjectPath(conn->path());
}

/*!
 \brief 释放此连接服务使用的指定资源引用
 当一个服务引用了多个资源时,此方法只会释放指定资源的引用,不会影响此服务的其它资源的引用情况.
 \a service 服务名称,关联特定进程
 */
void DSGConfigServer::onReleaseChanged(const ConnServiceName &service, const ConnKey &connKey)
{
    m_refManager->derefResource(service, connKey);

    const auto remainingCount = m_refManager->getRefResourceCountOnTheSR(service, connKey);
    qCInfo(cfLog, "Reduced connection reference service. service:%s, path:%s, remaining reference %d", qPrintable(service), qPrintable(connKey), remainingCount);
}

/*!
 \brief 释放此连接的所有资源
 \a resource 连接名称,一个配置资源的ID
 */
void DSGConfigServer::onReleaseResource(const ConnKey &connKey)
{
    const GenericResourceKey &resourceKey = getGenericResourceKey(connKey);
    auto resource = m_resources.value(resourceKey);
    if (!resource)
        return;
    qCInfo(cfLog, "Remove connection:%s", qPrintable(connKey));
    resource->removeConn(connKey);

    if (resource->isEmptyConn()) {
        qCInfo(cfLog, "Remove resource:%s", qPrintable(resourceKey));

        m_resources.remove(resourceKey);
        resource->deleteLater();

        if (m_enableExit) {
            Q_EMIT tryExit();
        }
    }
}

void DSGConfigServer::onTryExit()
{
    const int count = resourceSize();
    qCDebug(cfLog, "Try exit application, resource size:%d", count);

    if (count <= 0) {
        qCInfo(cfLog()) << "Exit application because of not exist resource.";
        exit();
        qApp->quit();
    }
}

void DSGConfigServer::doSyncConfigCache(const ConfigSyncBatchRequest &request)
{
    const QList<ConfigCacheKey> &keys = request.data;
    qCInfo(cfLog, "Do sync config cache, keys count:%d", keys.size());
    for (auto key: keys) {
        auto resourceKey = getResourceKeyByConfigCache(key);
        const auto genericResourceKey = getGenericResourceKeyByResourceKey(resourceKey);
        if (auto resource = m_resources.value(genericResourceKey)) {
            resource->doSyncConfigCache(key);
        }
    }
}

ResourceKey DSGConfigServer::getResourceKeyByConfigCache(const ConfigCacheKey &key)
{
    if (ConfigSyncRequestCache::isUserKey(key)) {
        return getResourceKey(ConfigSyncRequestCache::getUserKey(key));
    } else if (ConfigSyncRequestCache::isGlobalKey(key)){
        return ConfigSyncRequestCache::getGlobalKey(key);
    }
    return ResourceKey();
}

ConfigureId DSGConfigServer::getConfigureIdByPath(const QString &path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        return ConfigureId();
    }

    const auto &absolutePath = fileInfo.absoluteFilePath();

    auto res = getMetaConfigureId(absolutePath);
    if (res.isInValid()) {
        res = getOverrideConfigureId(absolutePath);
    }
    if (!res.isInValid()) {
        if (isConfigurePath(absolutePath, res.appid))
            return res;
    }
    return ConfigureId();
}

static bool isPathInDirectory(const QString &path, const QString &dir) {
    const QDir targetDir(dir);
    QDir currentDir(path);
    while (currentDir != targetDir && !currentDir.isRoot()) {
        if (!currentDir.cdUp()) {
            return false;
        }
    }

    return (currentDir == targetDir);
}

bool DSGConfigServer::isConfigurePath(const QString &path, const QString &appId) const
{
    QStringList dirs;
    const QStringList metaDirs = DConfigMeta::genericMetaDirs(m_localPrefix);
    dirs << metaDirs;

    if (!appId.isEmpty())
        dirs << DConfigMeta::applicationMetaDirs(m_localPrefix, appId);

    QStringList overrideDirs {
        QString("%1/etc/dsg/configs/overrides").arg(m_localPrefix)
    };
    for (const auto dir : metaDirs) {
        overrideDirs << QString("%1/%2/overrides").arg(m_localPrefix).arg(dir);
    }
    dirs << overrideDirs;

    for (const auto dir: dirs) {
        if (isPathInDirectory(path, dir)) {
            return true;
        }
    }
    return false;
}

/*!
 \brief 文件刷新，
 当描述文件被修改或override目录新增、移除、修改文件时，需要重新解析对应的文件内容，
 提供刷新服务，由配置工具调用来运行时刷新提供的文件访问信息。
 */
void DSGConfigServer::update(const QString &path)
{
    qCInfo(cfLog()) << "Update resource:" << path;

    const auto &configureInfo = getConfigureIdByPath(path);
    qCInfo(cfLog, "Update the configuration: appid:[%s], subpath:[%s], configurationid:[%s].",
           qPrintable(configureInfo.appid),
           qPrintable(configureInfo.subpath),
           qPrintable(configureInfo.resource));
    if (configureInfo.isInValid()) {
        QString errorMsg = QString("It's illegal resource [%1].").arg(path);
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::Failed, errorMsg);
        }
        qWarning() << errorMsg;
        return;
    }


    const GenericResourceKey resourceKey = getGenericResourceKey(configureInfo.resource, configureInfo.subpath);
    if (auto resource = resourceObject(resourceKey)) {
        qCInfo(cfLog, "Updated the resouce:[%s], for the appid:[%s].",
               qPrintable(resourceKey),
               qPrintable(configureInfo.appid));
        const auto &innerAppid = outerAppidToInner(configureInfo.appid);
        if (!resource->reparse(innerAppid)) {
            QString errorMsg = QString("Update the resource path[%1] error.").arg(path);
            if (calledFromDBus()) {
                sendErrorReply(QDBusError::Failed, errorMsg);
            }
            qWarning() << qPrintable(errorMsg);
        }
    }
}

void DSGConfigServer::sync(const QString &path)
{
    qInfo() << "Sync the resource:" << path;

    const auto &configureInfo = getConfigureIdByPath(path);
    if (configureInfo.isInValid()) {
        QString errorMsg = QString("It's illegal resource [%1].").arg(path);
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::Failed, errorMsg);
        }
        qWarning() << qPrintable(errorMsg);
        return;
    }

    qCInfo(cfLog, "Sync the configuration: appid:[%s], subpath:[%s], configurationid:[%s].",
           qPrintable(configureInfo.appid),
           qPrintable(configureInfo.subpath),
           qPrintable(configureInfo.resource));
    const GenericResourceKey resourceKey = getGenericResourceKey(configureInfo.resource, configureInfo.subpath);
    if (auto resource = resourceObject(resourceKey)) {
        qCInfo(cfLog, "Sync the resouce:[%s], for the appid:[%s].", qPrintable(resourceKey), qPrintable(configureInfo.appid));
        const auto &innerAppid = outerAppidToInner(configureInfo.appid);
        resource->save(innerAppid);
    }
}

/*!
 \brief 添加连接服务监控
 * 当服务退出时,会清空此服务的所有引用资源,即使服务异常退出,DBus也可以检测到.
 \a service 服务名称
 */
void DSGConfigServer::addConnWatchedService(const ConnServiceName & service)
{
    if (!calledFromDBus()) {
        return;
    }
    if (!m_watcher) {
        m_watcher = new QDBusServiceWatcher(this);
        m_watcher->setConnection(connection());
        m_watcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, [this](const QString &service){

            qCInfo(cfLog, "Remove watchered service:%s", qPrintable(service));
            m_watcher->removeWatchedService(service);
            m_refManager->releaseService(service);
        });
    }
    if (!m_watcher->watchedServices().contains(service)) {
        qCInfo(cfLog, "Add watchered service:%s, application:%s, user:%s.",
                qPrintable(service),
                qPrintable(getProcessNameByPid(connection().interface()->servicePid(service).value())),
                qPrintable(getUserNameByUid(connection().interface()->serviceUid(service).value())));
        m_watcher->addWatchedService(service);
    }
}

/*!
 * \brief Reload configuration files by detecting changes and updating them
 *
 */
void DSGConfigServer::reload()
{
    qCInfo(cfLog()) << "Reload configuration files";
    
    const auto lastSignatures = m_fileSignatures;
    m_fileSignatures = allConfigureFileSignatures(m_localPrefix);

    // Find changed files
    auto diffConfigureFiles = [] (const QVector<FileSignature> &s1, const QVector<FileSignature> &s2) {
        QStringList diffs;
        for (const auto& item : std::as_const(s1)) {
            auto iter = std::find_if(s2.cbegin(), s2.cend(), [&item](const FileSignature& other) {
                return item.filePath == other.filePath;
            });
            if (iter == s2.end() || (iter->changeTime != item.changeTime || iter->size != item.size)) {
                diffs << item.filePath;
            }
        }
        return diffs;
    };

    QStringList changedFiles;
    changedFiles << diffConfigureFiles(lastSignatures, m_fileSignatures);
    changedFiles << diffConfigureFiles(m_fileSignatures, lastSignatures);

    changedFiles.removeDuplicates();

    // Process changed files
    for (const auto &file : std::as_const(changedFiles)) {
        update(file);
    }

    qCInfo(cfLog()) << "Reload completed, processed" << changedFiles.size() << "files";
}

// Get all configuration file signatures
QVector<DSGConfigServer::FileSignature> DSGConfigServer::allConfigureFileSignatures(const QString &localPrefix)
{
    QVector<DSGConfigServer::FileSignature> signatures;

    QStringList dirs;
    // Get generic configuration directories
    const QStringList metaDirs = DConfigMeta::genericMetaDirs(localPrefix);
    dirs << metaDirs;

    // Get override directories
    QStringList overrideDirs {
        QString("%1/etc/dsg/configs/overrides").arg(localPrefix)
    };
    for (const auto &dir : std::as_const(metaDirs)) {
        overrideDirs << QString("%1/overrides").arg(dir);
    }
    dirs << overrideDirs;

    for (const QString &dir : std::as_const(dirs)) {
        if (!QDir(dir).exists())
            continue;

        QDirIterator iterator(dir, QStringList() << "*.json",
                             QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            const QString filePath = iterator.fileInfo().absoluteFilePath();
            
            QFileInfo fileInfo(filePath);
            if (fileInfo.exists()) {
                DSGConfigServer::FileSignature signature;
                signature.filePath = filePath;
                signature.size = fileInfo.size();
                signature.changeTime = fileInfo.metadataChangeTime(QTimeZone::UTC);

                signatures << signature;
            }
        }
    }

    return signatures;
}
