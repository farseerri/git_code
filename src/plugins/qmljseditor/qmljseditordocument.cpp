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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljseditordocument.h"

#include "qmljseditordocument_p.h"
#include "qmljshighlighter.h"
#include "qmljssemantichighlighter.h"
#include "qmljssemanticinfoupdater.h"

#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <qmljstools/qmljsqtstylecodeformatter.h>

using namespace QmlJSEditor;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSTools;

namespace {

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 100
};

class FindIdDeclarations: protected Visitor
{
public:
    typedef QHash<QString, QList<AST::SourceLocation> > Result;

    Result operator()(Document::Ptr doc)
    {
        _ids.clear();
        _maybeIds.clear();
        if (doc && doc->qmlProgram())
            doc->qmlProgram()->accept(this);
        return _ids;
    }

protected:
    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (!id->name.isEmpty())
                text += id->name;
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;
    using Visitor::endVisit;

    virtual bool visit(AST::UiScriptBinding *node)
    {
        if (asString(node->qualifiedId) == QLatin1String("id")) {
            if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement*>(node->statement)) {
                if (AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression *>(stmt->expression)) {
                    if (!idExpr->name.isEmpty()) {
                        const QString &id = idExpr->name.toString();
                        QList<AST::SourceLocation> *locs = &_ids[id];
                        locs->append(idExpr->firstSourceLocation());
                        locs->append(_maybeIds.value(id));
                        _maybeIds.remove(id);
                        return false;
                    }
                }
            }
        }

        accept(node->statement);

        return false;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (!node->name.isEmpty()) {
            const QString &name = node->name.toString();

            if (_ids.contains(name))
                _ids[name].append(node->identifierToken);
            else
                _maybeIds[name].append(node->identifierToken);
        }
        return false;
    }

private:
    Result _ids;
    Result _maybeIds;
};

class FindDeclarations: protected Visitor
{
    QList<Declaration> _declarations;
    int _depth;

public:
    QList<Declaration> operator()(AST::Node *node)
    {
        _depth = -1;
        _declarations.clear();
        accept(node);
        return _declarations;
    }

protected:
    using Visitor::visit;
    using Visitor::endVisit;

    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (!id->name.isEmpty())
                text += id->name;
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    void init(Declaration *decl, AST::UiObjectMember *member)
    {
        const SourceLocation first = member->firstSourceLocation();
        const SourceLocation last = member->lastSourceLocation();
        decl->startLine = first.startLine;
        decl->startColumn = first.startColumn;
        decl->endLine = last.startLine;
        decl->endColumn = last.startColumn + last.length;
    }

    void init(Declaration *decl, AST::ExpressionNode *expressionNode)
    {
        const SourceLocation first = expressionNode->firstSourceLocation();
        const SourceLocation last = expressionNode->lastSourceLocation();
        decl->startLine = first.startLine;
        decl->startColumn = first.startColumn;
        decl->endLine = last.startLine;
        decl->endColumn = last.startColumn + last.length;
    }

    virtual bool visit(AST::UiObjectDefinition *node)
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    virtual void endVisit(AST::UiObjectDefinition *)
    {
        --_depth;
    }

    virtual bool visit(AST::UiObjectBinding *node)
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);

        decl.text.append(asString(node->qualifiedId));
        decl.text.append(QLatin1String(": "));

        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    virtual void endVisit(AST::UiObjectBinding *)
    {
        --_depth;
    }

    virtual bool visit(AST::UiScriptBinding *)
    {
        ++_depth;

#if 0 // ### ignore script bindings for now.
        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text.append(asString(node->qualifiedId));

        _declarations.append(decl);
#endif

        return false; // more more bindings in this subtree.
    }

    virtual void endVisit(AST::UiScriptBinding *)
    {
        --_depth;
    }

    virtual bool visit(AST::FunctionExpression *)
    {
        return false;
    }

    virtual bool visit(AST::FunctionDeclaration *ast)
    {
        if (ast->name.isEmpty())
            return false;

        Declaration decl;
        init(&decl, ast);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name;

        decl.text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (!it->name.isEmpty())
                decl.text += it->name;

            if (it->next)
                decl.text += QLatin1String(", ");
        }

        decl.text += QLatin1Char(')');

        _declarations.append(decl);

        return false;
    }

    virtual bool visit(AST::VariableDeclaration *ast)
    {
        if (ast->name.isEmpty())
            return false;

        Declaration decl;
        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name;

        const SourceLocation first = ast->identifierToken;
        decl.startLine = first.startLine;
        decl.startColumn = first.startColumn;
        decl.endLine = first.startLine;
        decl.endColumn = first.startColumn + first.length;

        _declarations.append(decl);

        return false;
    }
};

class CreateRanges: protected AST::Visitor
{
    QTextDocument *_textDocument;
    QList<Range> _ranges;

public:
    QList<Range> operator()(QTextDocument *textDocument, Document::Ptr doc)
    {
        _textDocument = textDocument;
        _ranges.clear();
        if (doc && doc->ast() != 0)
            doc->ast()->accept(this);
        return _ranges;
    }

protected:
    using AST::Visitor::visit;

    virtual bool visit(AST::UiObjectBinding *ast)
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    virtual bool visit(AST::UiObjectDefinition *ast)
    {
        if (ast->initializer && ast->initializer->lbraceToken.length)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    virtual bool visit(AST::FunctionExpression *ast)
    {
        _ranges.append(createRange(ast));
        return true;
    }

    virtual bool visit(AST::FunctionDeclaration *ast)
    {
        _ranges.append(createRange(ast));
        return true;
    }

    virtual bool visit(AST::UiScriptBinding *ast)
    {
        if (AST::Block *block = AST::cast<AST::Block *>(ast->statement))
            _ranges.append(createRange(ast, block));
        return true;
    }

    Range createRange(AST::UiObjectMember *member, AST::UiObjectInitializer *ast)
    {
        return createRange(member, member->firstSourceLocation(), ast->rbraceToken);
    }

    Range createRange(AST::FunctionExpression *ast)
    {
        return createRange(ast, ast->lbraceToken, ast->rbraceToken);
    }

    Range createRange(AST::UiScriptBinding *ast, AST::Block *block)
    {
        return createRange(ast, block->lbraceToken, block->rbraceToken);
    }

    Range createRange(AST::Node *ast, AST::SourceLocation start, AST::SourceLocation end)
    {
        Range range;

        range.ast = ast;

        range.begin = QTextCursor(_textDocument);
        range.begin.setPosition(start.begin());

        range.end = QTextCursor(_textDocument);
        range.end.setPosition(end.end());

        return range;
    }
};

}

namespace QmlJSEditor {
namespace Internal {

QmlJSEditorDocumentPrivate::QmlJSEditorDocumentPrivate(QmlJSEditorDocument *parent)
    : m_q(parent),
      m_semanticInfoDocRevision(-1),
      m_semanticHighlighter(new SemanticHighlighter(parent)),
      m_semanticHighlightingNecessary(false)
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    // code model
    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_q->document(), SIGNAL(contentsChanged()), m_updateDocumentTimer, SLOT(start()));
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(reparseDocument()));
    connect(modelManager, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
            this, SLOT(onDocumentUpdated(QmlJS::Document::Ptr)));

    // semantic info
    m_semanticInfoUpdater = new SemanticInfoUpdater(this);
    connect(m_semanticInfoUpdater, SIGNAL(updated(QmlJSTools::SemanticInfo)),
            this, SLOT(acceptNewSemanticInfo(QmlJSTools::SemanticInfo)));
    m_semanticInfoUpdater->start();

    // library info changes
    m_reupdateSemanticInfoTimer = new QTimer(this);
    m_reupdateSemanticInfoTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_reupdateSemanticInfoTimer->setSingleShot(true);
    connect(m_reupdateSemanticInfoTimer, SIGNAL(timeout()), this, SLOT(reupdateSemanticInfo()));
    connect(modelManager, SIGNAL(libraryInfoUpdated(QString,QmlJS::LibraryInfo)),
            m_reupdateSemanticInfoTimer, SLOT(start()));
}

QmlJSEditorDocumentPrivate::~QmlJSEditorDocumentPrivate()
{
    m_semanticInfoUpdater->abort();
    m_semanticInfoUpdater->wait();
}

void QmlJSEditorDocumentPrivate::invalidateFormatterCache()
{
    CreatorCodeFormatter formatter(m_q->tabSettings());
    formatter.invalidateCache(m_q->document());
}

void QmlJSEditorDocumentPrivate::reparseDocument()
{
    ModelManagerInterface::instance()->updateSourceFiles(QStringList() << m_q->filePath(),
                                                                false);
}

void QmlJSEditorDocumentPrivate::onDocumentUpdated(Document::Ptr doc)
{
    if (m_q->filePath() != doc->fileName())
        return;

    // text document has changed, simply wait for the next onDocumentUpdated
    if (doc->editorRevision() != m_q->document()->revision())
        return;

    if (doc->ast()) {
        // got a correctly parsed (or recovered) file.
        m_semanticInfoDocRevision = doc->editorRevision();
        m_semanticInfoUpdater->update(doc, ModelManagerInterface::instance()->snapshot());
    }
    emit m_q->updateCodeWarnings(doc);
}

void QmlJSEditorDocumentPrivate::reupdateSemanticInfo()
{
    // If the editor is newer than the semantic info (possibly with update in progress),
    // new semantic infos won't be accepted anyway. We'll get a onDocumentUpdated anyhow.
    if (m_q->document()->revision() != m_semanticInfoDocRevision)
        return;

    m_semanticInfoUpdater->reupdate(ModelManagerInterface::instance()->snapshot());
}

void QmlJSEditorDocumentPrivate::acceptNewSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision() != m_q->document()->revision()) {
        // ignore outdated semantic infos
        return;
    }

    m_semanticInfo = semanticInfo;
    Document::Ptr doc = semanticInfo.document;

    // create the ranges
    CreateRanges createRanges;
    m_semanticInfo.ranges = createRanges(m_q->document(), doc);

    // Refresh the ids
    FindIdDeclarations updateIds;
    m_semanticInfo.idLocations = updateIds(doc);

    m_semanticHighlightingNecessary = true;
    emit m_q->semanticInfoUpdated(m_semanticInfo);
}

QmlJSEditorDocument::QmlJSEditorDocument()
    : m_d(new QmlJSEditorDocumentPrivate(this))
{
    connect(this, SIGNAL(tabSettingsChanged()),
            m_d, SLOT(invalidateFormatterCache()));
    setSyntaxHighlighter(new Highlighter(document()));
    setIndenter(new Indenter);
}

QmlJSEditorDocument::~QmlJSEditorDocument()
{
    delete m_d;
}

const SemanticInfo &QmlJSEditorDocument::semanticInfo() const
{
    return m_d->m_semanticInfo;
}

bool QmlJSEditorDocument::isSemanticInfoOutdated() const
{
    return m_d->m_semanticInfo.revision() != document()->revision();
}

QVector<QTextLayout::FormatRange> QmlJSEditorDocument::diagnosticRanges() const
{
    return m_d->m_diagnosticRanges;
}

void QmlJSEditorDocument::setDiagnosticRanges(const QVector<QTextLayout::FormatRange> &ranges)
{
    m_d->m_diagnosticRanges = ranges;
}

void QmlJSEditorDocument::applyFontSettings()
{
    BaseTextDocument::applyFontSettings();
    m_d->m_semanticHighlighter->updateFontSettings(fontSettings());
    if (!isSemanticInfoOutdated()) {
        m_d->m_semanticHighlightingNecessary = false;
        m_d->m_semanticHighlighter->rerun(m_d->m_semanticInfo);
    }
}

void QmlJSEditorDocument::triggerPendingUpdates()
{
    BaseTextDocument::triggerPendingUpdates(); // calls applyFontSettings if necessary
    // might still need to rehighlight if font settings did not change
    if (m_d->m_semanticHighlightingNecessary && !isSemanticInfoOutdated()) {
        m_d->m_semanticHighlightingNecessary = false;
        m_d->m_semanticHighlighter->rerun(m_d->m_semanticInfo);
    }
}

} // Internal
} // QmlJSEditor
