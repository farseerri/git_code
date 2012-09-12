/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QT4TARGETSETUPWIDGET_H
#define QT4TARGETSETUPWIDGET_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/task.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpacerItem;
QT_END_NAMESPACE

namespace ProjectExplorer { class Kit; }
namespace QtSupport {
class BaseQtVersion;
} // namespace QtSupport
namespace Utils {
class DetailsWidget;
class PathChooser;
} // namespace Utils

namespace Qt4ProjectManager {
class BuildConfigurationInfo;

class QT4PROJECTMANAGER_EXPORT Qt4TargetSetupWidget : public QWidget
{
    Q_OBJECT
public:
    Qt4TargetSetupWidget(ProjectExplorer::Kit *k,
                         const QString &proFilePath,
                         const QList<BuildConfigurationInfo> &infoList);
    ~Qt4TargetSetupWidget();

    ProjectExplorer::Kit *profile();
    void clearKit();

    bool isKitSelected() const;
    void setKitSelected(bool b);

    void addBuildConfigurationInfo(const BuildConfigurationInfo &info, bool importing = false);

    QList<BuildConfigurationInfo> selectedBuildConfigurationInfoList() const;
    QList<BuildConfigurationInfo> allBuildConfigurationInfoList() const;
    void setProFilePath(const QString &proFilePath);

signals:
    void selectedToggled() const;

private slots:
    void handleKitUpdate(ProjectExplorer::Kit *k);

    void checkBoxToggled(bool b);
    void pathChanged();
    void targetCheckBoxToggled(bool b);
    void manageKit();

private:
    void reportIssues(int index);
    QPair<ProjectExplorer::Task::TaskType, QString> findIssues(const BuildConfigurationInfo &info);
    void clear();

    ProjectExplorer::Kit *m_kit;
    QString m_proFilePath;
    bool m_haveImported;
    Utils::DetailsWidget *m_detailsWidget;
    QPushButton *m_manageButton;
    QGridLayout *m_newBuildsLayout;
    QList<QCheckBox *> m_checkboxes;
    QList<Utils::PathChooser *> m_pathChoosers;
    QList<BuildConfigurationInfo> m_infoList;
    QList<bool> m_enabled;
    QList<QLabel *> m_reportIssuesLabels;
    QList<bool> m_issues;
    bool m_ignoreChange;
    int m_selected; // Number of selected buildconfiguartions
};

} // namespace Qt4ProjectManager

#endif // QT4TARGETSETUPWIDGET_H
