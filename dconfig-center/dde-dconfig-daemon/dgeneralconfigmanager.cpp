// SPDX-FileCopyrightText: 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dgeneralconfigmanager.h"
#include "dconfigfile.h"
#include <QDebug>

InterappConfig::~InterappConfig()
{
    delete m_config;
    qDeleteAll(m_caches);
    m_caches.clear();
}

DConfigCache* InterappConfig::cache(const uint uid) const
{
    return m_caches.value(uid);
}

bool InterappConfig::contains(const uint id) const
{
    return m_caches.contains(id);
}

void InterappConfig::removeCache(const uint uid)
{
    const auto iter = m_caches.find(uid);
    if (iter != m_caches.end()) {
        delete *iter;
        m_caches.erase(iter);
    }
}

void InterappConfig::addCache(const uint uid, DConfigCache* cache)
{
    Q_ASSERT(cache);
    m_caches[uid] = cache;
}

void InterappConfig::setConfig(DConfigFile* config)
{
    m_config = config;
}

DConfigFile* InterappConfig::config() const
{
    return m_config;
}

QList<DConfigCache *> InterappConfig::caches() const
{
    return m_caches.values();
}

DSGInterappConfigManager::DSGInterappConfigManager(QObject *parent)
    : QObject (parent)
{

}

DSGInterappConfigManager::~DSGInterappConfigManager()
{
    for (auto item : m_generalConfigs) {
        delete item;
    }
    m_generalConfigs.clear();
}

InterappConfig *DSGInterappConfigManager::config(const InterappConfigFileKey &key) const
{
    return m_generalConfigs.value(key);
}

bool DSGInterappConfigManager::contains(const InterappConfigFileKey &key) const
{
    return m_generalConfigs.contains(key);
}

InterappConfig *DSGInterappConfigManager::createConfig(const InterappConfigFileKey &key)
{
    auto iter = m_generalConfigs.find(key);
    if (iter != m_generalConfigs.end())
        return *iter;

    auto config = new InterappConfig();
    m_generalConfigs[key] = config;
    return config;
}

void DSGInterappConfigManager::removeConfig(const InterappConfigFileKey &key)
{
    const auto iter = m_generalConfigs.find(key);
    if (iter != m_generalConfigs.end()) {
        delete iter.value();
        m_generalConfigs.erase(iter);
    }
}
