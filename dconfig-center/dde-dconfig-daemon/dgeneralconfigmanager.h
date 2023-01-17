// SPDX-FileCopyrightText: 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include <dtkcore_global.h>
#include <DConfigFile>
#include <QObject>

DCORE_BEGIN_NAMESPACE
class DConfigFile;
class DConfigCache;
DCORE_END_NAMESPACE

DCORE_USE_NAMESPACE
/**
 * @brief The InterappConfig class
 * 管理单个资源的公共配置信息
 * 与DSGConfigResource类似，含有一个DConfigFile及多个DConfigCache.
 */
class InterappConfig
{
public:
    ~InterappConfig();
    DConfigCache* cache(const uint uid) const;
    bool contains(const uint id) const;
    void removeCache(const uint uid);
    void addCache(const uint uid, DConfigCache* cache);
    void setConfig(DConfigFile* config);
    DConfigFile* config() const;
    QList<DConfigCache *> caches() const;
private:
    DConfigFile* m_config = nullptr;
    QMap<uint, DConfigCache*> m_caches;
};

/**
 * @brief The DSGInterappConfigManager class
 * 管理公共资源
 */
class DSGInterappConfigManager : public QObject
{
    Q_OBJECT
public:
    DSGInterappConfigManager(QObject *parent = nullptr);
    virtual ~DSGInterappConfigManager() override;

    InterappConfig *config(const InterappConfigFileKey &key) const;
    bool contains(const InterappConfigFileKey &key) const;
    InterappConfig *createConfig(const InterappConfigFileKey &key);
    void removeConfig(const InterappConfigFileKey &key);

Q_SIGNALS:
    void valueChanged(const QString &key, const ConnKey &connKey);

private:
    QMap<InterappConfigFileKey, InterappConfig *> m_generalConfigs;
};
