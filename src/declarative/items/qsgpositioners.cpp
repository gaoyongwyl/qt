// Commit: 4b68c14af425a3f8441ae0377c178d398192d45a
/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgpositioners_p.h"
#include "qsgpositioners_p_p.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtCore/qmath.h>
#include <QtCore/qcoreapplication.h>

#include <private/qdeclarativestate_p.h>
#include <private/qdeclarativestategroup_p.h>
#include <private/qdeclarativestateoperations_p.h>

QT_BEGIN_NAMESPACE

static const QSGItemPrivate::ChangeTypes watchedChanges
    = QSGItemPrivate::Geometry
    | QSGItemPrivate::SiblingOrder
    | QSGItemPrivate::Visibility
    | QSGItemPrivate::Opacity
    | QSGItemPrivate::Destroyed;

void QSGBasePositionerPrivate::watchChanges(QSGItem *other)
{
    QSGItemPrivate *otherPrivate = QSGItemPrivate::get(other);
    otherPrivate->addItemChangeListener(this, watchedChanges);
}

void QSGBasePositionerPrivate::unwatchChanges(QSGItem* other)
{
    QSGItemPrivate *otherPrivate = QSGItemPrivate::get(other);
    otherPrivate->removeItemChangeListener(this, watchedChanges);
}

QSGBasePositioner::QSGBasePositioner(PositionerType at, QSGItem *parent)
    : QSGItem(*(new QSGBasePositionerPrivate), parent)
{
    Q_D(QSGBasePositioner);
    d->init(at);
}

QSGBasePositioner::QSGBasePositioner(QSGBasePositionerPrivate &dd, PositionerType at, QSGItem *parent)
    : QSGItem(dd, parent)
{
    Q_D(QSGBasePositioner);
    d->init(at);
}

QSGBasePositioner::~QSGBasePositioner()
{
    Q_D(QSGBasePositioner);
    for (int i = 0; i < positionedItems.count(); ++i)
        d->unwatchChanges(positionedItems.at(i).item);
    positionedItems.clear();
}

int QSGBasePositioner::spacing() const
{
    Q_D(const QSGBasePositioner);
    return d->spacing;
}

void QSGBasePositioner::setSpacing(int s)
{
    Q_D(QSGBasePositioner);
    if (s==d->spacing)
        return;
    d->spacing = s;
    prePositioning();
    emit spacingChanged();
}

QDeclarativeTransition *QSGBasePositioner::move() const
{
    Q_D(const QSGBasePositioner);
    return d->moveTransition;
}

void QSGBasePositioner::setMove(QDeclarativeTransition *mt)
{
    Q_D(QSGBasePositioner);
    if (mt == d->moveTransition)
        return;
    d->moveTransition = mt;
    emit moveChanged();
}

QDeclarativeTransition *QSGBasePositioner::add() const
{
    Q_D(const QSGBasePositioner);
    return d->addTransition;
}

void QSGBasePositioner::setAdd(QDeclarativeTransition *add)
{
    Q_D(QSGBasePositioner);
    if (add == d->addTransition)
        return;

    d->addTransition = add;
    emit addChanged();
}

void QSGBasePositioner::componentComplete()
{
    QSGItem::componentComplete();
    positionedItems.reserve(childItems().count());
    prePositioning();
    reportConflictingAnchors();
}

void QSGBasePositioner::itemChange(GraphicsItemChange change, const QVariant &value)
{
    Q_D(QSGBasePositioner);
    if (change == ItemChildAddedChange){
        prePositioning();
    } else if (change == ItemChildRemovedChange) {
        QSGItem *child = value.value<QSGItem*>();
        QSGBasePositioner::PositionedItem posItem(child);
        int idx = positionedItems.find(posItem);
        if (idx >= 0) {
            d->unwatchChanges(child);
            positionedItems.remove(idx);
        }
        prePositioning();
    }

    QSGItem::itemChange(change, value);
}

void QSGBasePositioner::prePositioning()
{
    Q_D(QSGBasePositioner);
    if (!isComponentComplete())
        return;

    if (d->doingPositioning)
        return;

    d->queuedPositioning = false;
    d->doingPositioning = true;
    //Need to order children by creation order modified by stacking order
    QList<QSGItem *> children = childItems();

    QPODVector<PositionedItem,8> oldItems;
    positionedItems.copyAndClear(oldItems);
    for (int ii = 0; ii < children.count(); ++ii) {
        QSGItem *child = children.at(ii);
        QSGItemPrivate *childPrivate = QSGItemPrivate::get(child);
        PositionedItem *item = 0;
        PositionedItem posItem(child);
        int wIdx = oldItems.find(posItem);
        if (wIdx < 0) {
            d->watchChanges(child);
            positionedItems.append(posItem);
            item = &positionedItems[positionedItems.count()-1];
            item->isNew = true;
            if (child->opacity() <= 0.0 || !childPrivate->explicitVisible || !child->width() || !child->height())
                item->isVisible = false;
        } else {
            item = &oldItems[wIdx];
            // Items are only omitted from positioning if they are explicitly hidden
            // i.e. their positioning is not affected if an ancestor is hidden.
            if (child->opacity() <= 0.0 || !childPrivate->explicitVisible || !child->width() || !child->height()) {
                item->isVisible = false;
            } else if (!item->isVisible) {
                item->isVisible = true;
                item->isNew = true;
            } else {
                item->isNew = false;
            }
            positionedItems.append(*item);
        }
    }
    QSizeF contentSize;
    doPositioning(&contentSize);
    if(d->addTransition || d->moveTransition)
        finishApplyTransitions();
    d->doingPositioning = false;
    //Set implicit size to the size of its children
    setImplicitHeight(contentSize.height());
    setImplicitWidth(contentSize.width());
}

void QSGBasePositioner::positionX(int x, const PositionedItem &target)
{
    Q_D(QSGBasePositioner);
    if(d->type == Horizontal || d->type == Both){
        if (target.isNew) {
            if (!d->addTransition)
                target.item->setX(x);
            else
                d->addActions << QDeclarativeAction(target.item, QLatin1String("x"), QVariant(x));
        } else if (x != target.item->x()) {
            if (!d->moveTransition)
                target.item->setX(x);
            else
                d->moveActions << QDeclarativeAction(target.item, QLatin1String("x"), QVariant(x));
        }
    }
}

void QSGBasePositioner::positionY(int y, const PositionedItem &target)
{
    Q_D(QSGBasePositioner);
    if(d->type == Vertical || d->type == Both){
        if (target.isNew) {
            if (!d->addTransition)
                target.item->setY(y);
            else
                d->addActions << QDeclarativeAction(target.item, QLatin1String("y"), QVariant(y));
        } else if (y != target.item->y()) {
            if (!d->moveTransition)
                target.item->setY(y);
            else
                d->moveActions << QDeclarativeAction(target.item, QLatin1String("y"), QVariant(y));
        }
    }
}

void QSGBasePositioner::finishApplyTransitions()
{
    Q_D(QSGBasePositioner);
    // Note that if a transition is not set the transition manager will
    // apply the changes directly, in the case add/move aren't set
    d->addTransitionManager.transition(d->addActions, d->addTransition);
    d->moveTransitionManager.transition(d->moveActions, d->moveTransition);
    d->addActions.clear();
    d->moveActions.clear();
}

QSGColumn::QSGColumn(QSGItem *parent)
: QSGBasePositioner(Vertical, parent)
{
}

void QSGColumn::doPositioning(QSizeF *contentSize)
{
    int voffset = 0;

    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (!child.item || !child.isVisible)
            continue;

        if(child.item->y() != voffset)
            positionY(voffset, child);

        contentSize->setWidth(qMax(contentSize->width(), child.item->width()));

        voffset += child.item->height();
        voffset += spacing();
    }

    contentSize->setHeight(voffset - spacing());
}

void QSGColumn::reportConflictingAnchors()
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors) {
                QSGAnchors::Anchors usedAnchors = anchors->usedAnchors();
                if (usedAnchors & QSGAnchors::TopAnchor ||
                    usedAnchors & QSGAnchors::BottomAnchor ||
                    usedAnchors & QSGAnchors::VCenterAnchor ||
                    anchors->fill() || anchors->centerIn()) {
                    d->anchorConflict = true;
                    break;
                }
            }
        }
    }
    if (d->anchorConflict) {
        qmlInfo(this) << "Cannot specify top, bottom, verticalCenter, fill or centerIn anchors for items inside Column";
    }
}

QSGRow::QSGRow(QSGItem *parent)
: QSGBasePositioner(Horizontal, parent)
{
}

void QSGRow::doPositioning(QSizeF *contentSize)
{
    int hoffset = 0;

    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (!child.item || !child.isVisible)
            continue;

        if(child.item->x() != hoffset)
            positionX(hoffset, child);

        contentSize->setHeight(qMax(contentSize->height(), child.item->height()));

        hoffset += child.item->width();
        hoffset += spacing();
    }

    contentSize->setWidth(hoffset - spacing());
}

void QSGRow::reportConflictingAnchors()
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors) {
                QSGAnchors::Anchors usedAnchors = anchors->usedAnchors();
                if (usedAnchors & QSGAnchors::LeftAnchor ||
                    usedAnchors & QSGAnchors::RightAnchor ||
                    usedAnchors & QSGAnchors::HCenterAnchor ||
                    anchors->fill() || anchors->centerIn()) {
                    d->anchorConflict = true;
                    break;
                }
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify left, right, horizontalCenter, fill or centerIn anchors for items inside Row";
}

QSGGrid::QSGGrid(QSGItem *parent) :
    QSGBasePositioner(Both, parent), m_rows(-1), m_columns(-1), m_flow(LeftToRight)
{
}

void QSGGrid::setColumns(const int columns)
{
    if (columns == m_columns)
        return;
    m_columns = columns;
    prePositioning();
    emit columnsChanged();
}

void QSGGrid::setRows(const int rows)
{
    if (rows == m_rows)
        return;
    m_rows = rows;
    prePositioning();
    emit rowsChanged();
}

QSGGrid::Flow QSGGrid::flow() const
{
    return m_flow;
}

void QSGGrid::setFlow(Flow flow)
{
    if (m_flow != flow) {
        m_flow = flow;
        prePositioning();
        emit flowChanged();
    }
}

void QSGGrid::doPositioning(QSizeF *contentSize)
{

    int c = m_columns;
    int r = m_rows;
    //Is allocating the extra QPODVector too much overhead?
    QPODVector<PositionedItem, 8> visibleItems;//we aren't concerned with invisible items
    visibleItems.reserve(positionedItems.count());
    for(int i=0; i<positionedItems.count(); i++)
        if(positionedItems[i].item && positionedItems[i].isVisible)
            visibleItems.append(positionedItems[i]);

    int numVisible = visibleItems.count();
    if (m_columns <= 0 && m_rows <= 0){
        c = 4;
        r = (numVisible+3)/4;
    } else if (m_rows <= 0){
        r = (numVisible+(m_columns-1))/m_columns;
    } else if (m_columns <= 0){
        c = (numVisible+(m_rows-1))/m_rows;
    }

    QList<int> maxColWidth;
    QList<int> maxRowHeight;
    int childIndex =0;
    if (m_flow == LeftToRight) {
        for (int i=0; i < r; i++){
            for (int j=0; j < c; j++){
                if (j==0)
                    maxRowHeight << 0;
                if (i==0)
                    maxColWidth << 0;

                if (childIndex == visibleItems.count())
                    break;

                const PositionedItem &child = visibleItems.at(childIndex++);
                if (child.item->width() > maxColWidth[j])
                    maxColWidth[j] = child.item->width();
                if (child.item->height() > maxRowHeight[i])
                    maxRowHeight[i] = child.item->height();
            }
        }
    } else {
        for (int j=0; j < c; j++){
            for (int i=0; i < r; i++){
                if (j==0)
                    maxRowHeight << 0;
                if (i==0)
                    maxColWidth << 0;

                if (childIndex == positionedItems.count())
                    break;

                const PositionedItem &child = visibleItems.at(childIndex++);
                if (child.item->width() > maxColWidth[j])
                    maxColWidth[j] = child.item->width();
                if (child.item->height() > maxRowHeight[i])
                    maxRowHeight[i] = child.item->height();
            }
        }
    }

    int xoffset=0;
    int yoffset=0;
    int curRow =0;
    int curCol =0;
    for (int i = 0; i < visibleItems.count(); ++i) {
        const PositionedItem &child = visibleItems.at(i);
        if((child.item->x()!=xoffset)||(child.item->y()!=yoffset)){
            positionX(xoffset, child);
            positionY(yoffset, child);
        }

        if (m_flow == LeftToRight) {
            contentSize->setWidth(qMax(contentSize->width(), xoffset + child.item->width()));
            contentSize->setHeight(yoffset + maxRowHeight[curRow]);

            xoffset+=maxColWidth[curCol]+spacing();
            curCol++;
            curCol%=c;
            if (!curCol){
                yoffset+=maxRowHeight[curRow]+spacing();
                xoffset=0;
                curRow++;
                if (curRow>=r)
                    break;
            }
        } else {
            contentSize->setHeight(qMax(contentSize->height(), yoffset + child.item->height()));
            contentSize->setWidth(xoffset + maxColWidth[curCol]);

            yoffset+=maxRowHeight[curRow]+spacing();
            curRow++;
            curRow%=r;
            if (!curRow){
                xoffset+=maxColWidth[curCol]+spacing();
                yoffset=0;
                curCol++;
                if (curCol>=c)
                    break;
            }
        }
    }
}

void QSGGrid::reportConflictingAnchors()
{
    QSGBasePositionerPrivate *d = static_cast<QSGBasePositionerPrivate*>(QSGBasePositionerPrivate::get(this));
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors && (anchors->usedAnchors() || anchors->fill() || anchors->centerIn())) {
                d->anchorConflict = true;
                break;
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify anchors for items inside Grid";
}

class QSGFlowPrivate : public QSGBasePositionerPrivate
{
    Q_DECLARE_PUBLIC(QSGFlow)

public:
    QSGFlowPrivate()
        : QSGBasePositionerPrivate(), flow(QSGFlow::LeftToRight)
    {}

    QSGFlow::Flow flow;
};

QSGFlow::QSGFlow(QSGItem *parent)
: QSGBasePositioner(*(new QSGFlowPrivate), Both, parent)
{
    Q_D(QSGFlow);
    // Flow layout requires relayout if its own size changes too.
    d->addItemChangeListener(d, QSGItemPrivate::Geometry);
}

QSGFlow::Flow QSGFlow::flow() const
{
    Q_D(const QSGFlow);
    return d->flow;
}

void QSGFlow::setFlow(Flow flow)
{
    Q_D(QSGFlow);
    if (d->flow != flow) {
        d->flow = flow;
        prePositioning();
        emit flowChanged();
    }
}

void QSGFlow::doPositioning(QSizeF *contentSize)
{
    Q_D(QSGFlow);

    int hoffset = 0;
    int voffset = 0;
    int linemax = 0;

    for (int i = 0; i < positionedItems.count(); ++i) {
        const PositionedItem &child = positionedItems.at(i);
        if (!child.item || !child.isVisible)
            continue;

        if (d->flow == LeftToRight)  {
            if (widthValid() && hoffset && hoffset + child.item->width() > width()) {
                hoffset = 0;
                voffset += linemax + spacing();
                linemax = 0;
            }
        } else {
            if (heightValid() && voffset && voffset + child.item->height() > height()) {
                voffset = 0;
                hoffset += linemax + spacing();
                linemax = 0;
            }
        }

        if(child.item->x() != hoffset || child.item->y() != voffset){
            positionX(hoffset, child);
            positionY(voffset, child);
        }

        contentSize->setWidth(qMax(contentSize->width(), hoffset + child.item->width()));
        contentSize->setHeight(qMax(contentSize->height(), voffset + child.item->height()));

        if (d->flow == LeftToRight)  {
            hoffset += child.item->width();
            hoffset += spacing();
            linemax = qMax(linemax, qCeil(child.item->height()));
        } else {
            voffset += child.item->height();
            voffset += spacing();
            linemax = qMax(linemax, qCeil(child.item->width()));
        }
    }
}

void QSGFlow::reportConflictingAnchors()
{
    Q_D(QSGFlow);
    for (int ii = 0; ii < positionedItems.count(); ++ii) {
        const PositionedItem &child = positionedItems.at(ii);
        if (child.item) {
            QSGAnchors *anchors = QSGItemPrivate::get(static_cast<QSGItem *>(child.item))->_anchors;
            if (anchors && (anchors->usedAnchors() || anchors->fill() || anchors->centerIn())) {
                d->anchorConflict = true;
                break;
            }
        }
    }
    if (d->anchorConflict)
        qmlInfo(this) << "Cannot specify anchors for items inside Flow";
}

QT_END_NAMESPACE