/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KITMODEL_H
#define KITMODEL_H

#include "projectexplorer_export.h"

#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Kit;
class KitFactory;
class KitManager;

namespace Internal {
class KitManagerConfigWidget;

class KitNode;

// --------------------------------------------------------------------------
// KitModel:
// --------------------------------------------------------------------------

class KitModel : public Utils::TreeModel
{
    Q_OBJECT

public:
    explicit KitModel(QBoxLayout *parentLayout, QObject *parent = 0);

    Kit *kit(const QModelIndex &);
    QModelIndex indexOf(Kit *k) const;

    void setDefaultKit(const QModelIndex &index);
    bool isDefaultKit(const QModelIndex &index);

    ProjectExplorer::Internal::KitManagerConfigWidget *widget(const QModelIndex &);

    bool isDirty() const;
    bool isDirty(Kit *k) const;

    void apply();

    void markForRemoval(Kit *k);
    Kit *markForAddition(Kit *baseKit);

signals:
    void kitStateChanged();

private slots:
    void addKit(ProjectExplorer::Kit *k);
    void updateKit(ProjectExplorer::Kit *k);
    void removeKit(ProjectExplorer::Kit *k);
    void changeDefaultKit();
    void validateKitNames();

private:
    QModelIndex index(KitNode *, int column = 0) const;
    KitNode *findWorkingCopy(Kit *k) const;
    KitNode *createNode(Kit *k);
    void setDefaultNode(KitNode *node);

    Utils::TreeItem *m_autoRoot;
    Utils::TreeItem *m_manualRoot;

    QList<KitNode *> m_toRemoveList;

    QBoxLayout *m_parentLayout;
    KitNode *m_defaultNode;

    bool m_keepUnique;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // KITMODEL_H
