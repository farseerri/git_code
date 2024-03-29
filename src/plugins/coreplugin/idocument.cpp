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

#include "idocument.h"

#include "infobar.h"

#include <utils/qtcassert.h>

#include <QFile>
#include <QFileInfo>

/*!
    \class Core::IDocument
    \brief The IDocument class describes a document that can be saved and reloaded.

    The most common use for implementing an IDocument subclass, is as a document for an IEditor
    implementation. Multiple editors can work in the same document instance, so the IDocument
    subclass should hold all data and functions that are independent from the specific
    IEditor instance, for example the content, highlighting information, the name of the
    corresponding file, and functions for saving and reloading the file.

    Each IDocument subclass works only with the corresponding IEditor subclasses that it
    was designed to work with.

    \mainclass
*/

/*!
    \fn QString Core::IDocument::filePath() const
    Returns the absolute path of the file that this document refers to. May be empty for
    non-file documents.
    \sa setFilePath()
*/

namespace Core {

namespace Internal {

class IDocumentPrivate
{
public:
    IDocumentPrivate() :
        infoBar(0),
        temporary(false),
        hasWriteWarning(false),
        restored(false)
    {
    }

    ~IDocumentPrivate()
    {
        delete infoBar;
    }

    Id id;
    QString mimeType;
    Utils::FileName filePath;
    QString displayName;
    QString autoSaveName;
    InfoBar *infoBar;
    bool temporary;
    bool hasWriteWarning;
    bool restored;
};

} // namespace Internal

IDocument::IDocument(QObject *parent) : QObject(parent),
    d(new Internal::IDocumentPrivate)
{
}

IDocument::~IDocument()
{
    removeAutoSaveFile();
    delete d;
}

void IDocument::setId(Id id)
{
    d->id = id;
}

Id IDocument::id() const
{
    QTC_CHECK(d->id.isValid());
    return d->id;
}

/*!
    Used for example by EditorManager::openEditorWithContents() to set the contents
    of this document.
    Returns if setting the contents was successful.
    The base implementation does nothing and returns false.
*/
bool IDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents)
    return false;
}

Utils::FileName IDocument::filePath() const
{
    return d->filePath;
}

IDocument::ReloadBehavior IDocument::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents && state == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

void IDocument::checkPermissions()
{
}

bool IDocument::shouldAutoSave() const
{
    return false;
}

bool IDocument::isFileReadOnly() const
{
    if (filePath().isEmpty())
        return false;
    return !filePath().toFileInfo().isWritable();
}

/*!
    Returns if the document is a temporary that should for example not be considered
    when saving/restoring the session state, recent files, etc. Defaults to false.
    \sa setTemporary()
*/
bool IDocument::isTemporary() const
{
    return d->temporary;
}

/*!
    Sets if the document is \a temporary.
    \sa isTemporary()
*/
void IDocument::setTemporary(bool temporary)
{
    d->temporary = temporary;
}

QString IDocument::mimeType() const
{
    return d->mimeType;
}

void IDocument::setMimeType(const QString &mimeType)
{
    if (d->mimeType != mimeType) {
        d->mimeType = mimeType;
        emit mimeTypeChanged();
    }
}

bool IDocument::autoSave(QString *errorString, const QString &fileName)
{
    if (!save(errorString, fileName, true))
        return false;
    d->autoSaveName = fileName;
    return true;
}

static const char kRestoredAutoSave[] = "RestoredAutoSave";

void IDocument::setRestoredFrom(const QString &name)
{
    d->autoSaveName = name;
    d->restored = true;
    InfoBarEntry info(Id(kRestoredAutoSave),
          tr("File was restored from auto-saved copy. "
             "Select Save to confirm or Revert to Saved to discard changes."));
    infoBar()->addInfo(info);
}

void IDocument::removeAutoSaveFile()
{
    if (!d->autoSaveName.isEmpty()) {
        QFile::remove(d->autoSaveName);
        d->autoSaveName.clear();
        if (d->restored) {
            d->restored = false;
            infoBar()->removeInfo(Id(kRestoredAutoSave));
        }
    }
}

bool IDocument::hasWriteWarning() const
{
    return d->hasWriteWarning;
}

void IDocument::setWriteWarning(bool has)
{
    d->hasWriteWarning = has;
}

InfoBar *IDocument::infoBar()
{
    if (!d->infoBar)
        d->infoBar = new InfoBar;
    return d->infoBar;
}

/*!
    Set absolute file path for this file to \a filePath. Can be empty.
    The default implementation sets the file name and sends filePathChanged() and changed()
    signals. Can be reimplemented by subclasses to do more.
    \sa filePath()
*/
void IDocument::setFilePath(const Utils::FileName &filePath)
{
    if (d->filePath == filePath)
        return;
    Utils::FileName oldName = d->filePath;
    d->filePath = filePath;
    emit filePathChanged(oldName, d->filePath);
    emit changed();
}

/*!
    Returns the string to display for this document, e.g. in the open document combo box
    and pane.
    \sa setDisplayName()
*/
QString IDocument::displayName() const
{
    if (!d->displayName.isEmpty())
        return d->displayName;
    return d->filePath.toFileInfo().fileName();
}

/*!
    Sets the string that is displayed for this document, e.g. in the open document combo box
    and pane, to \a name. Defaults to the file name of the file path for this document.
    You can reset the display name to the default by passing an empty string.
    \sa displayName()
    \sa filePath()
 */
void IDocument::setDisplayName(const QString &name)
{
    if (name == d->displayName)
        return;
    d->displayName = name;
    emit changed();
}

} // namespace Core
