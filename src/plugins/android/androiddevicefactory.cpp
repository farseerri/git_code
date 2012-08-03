/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "androiddevicefactory.h"
#include "androiddevice.h"

#include "androidconstants.h"
#include <coreplugin/id.h>

namespace Android {
namespace Internal {

AndroidDeviceFactory::AndroidDeviceFactory()
{
    setObjectName(QLatin1String("AndroidDeviceFactory"));
}

QString AndroidDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type == Constants::ANDROID_DEVICE_TYPE)
        return tr("Android Device");
    return QString();
}

QList<Core::Id> AndroidDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::ANDROID_DEVICE_TYPE);
}

bool AndroidDeviceFactory::canCreate() const
{
    return false;
}

ProjectExplorer::IDevice::Ptr AndroidDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id)
    return ProjectExplorer::IDevice::Ptr();
}

bool AndroidDeviceFactory::canRestore(const QVariantMap &map) const
{
    return ProjectExplorer::IDevice::typeFromMap(map) == Constants::ANDROID_DEVICE_TYPE;
}

ProjectExplorer::IDevice::Ptr AndroidDeviceFactory::restore(const QVariantMap &map) const
{
    Q_UNUSED(map)
    return ProjectExplorer::IDevice::Ptr(new AndroidDevice);
}

} // namespace Internal
} // namespace Android
