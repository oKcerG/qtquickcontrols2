/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickslider_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Slider
    \inherits Control
    \instantiates QQuickSlider
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-input
    \brief Used to select a value by sliding a handle along a track.

    \image qtquickcontrols2-slider.gif

    Slider is used to select a value by sliding a handle along a track.

    In the example below, custom \l from, \l value, and \l to values are set:

    \code
    Slider {
        from: 1
        value: 25
        to: 100
    }
    \endcode

    The \l position property is expressed as a fraction of the control's size,
    in the range \c {0.0 - 1.0}. The \l visualPosition property is
    the same, except that it is reversed in a
    \l {Right-to-left User Interfaces}{right-to-left} application. The
    visualPosition is useful for positioning the handle when styling Slider.
    In the example above, \l visualPosition will be \c 0.24 in a left-to-right
    application, and \c 0.76 in a right-to-left application.

    \sa {Customizing Slider}, {Input Controls}
*/

/*!
    \since QtQuick.Controls 2.2
    \qmlsignal QtQuick.Controls::Slider::moved()

    This signal is emitted when the slider has been interactively moved
    by the user by either touch, mouse, wheel, or keys.
*/

class QQuickSliderPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickSlider)

public:
    QQuickSliderPrivate()
        : from(0),
          to(1),
          value(0),
          position(0),
          stepSize(0),
          live(true),
          pressed(false),
          touchId(-1),
          orientation(Qt::Horizontal),
          snapMode(QQuickSlider::NoSnap),
          handle(nullptr)
    {
    }

    qreal snapPosition(qreal position) const;
    qreal positionAt(const QPointF &point) const;
    void setPosition(qreal position);
    void updatePosition();

    void handlePress(const QPointF &point);
    void handleMove(const QPointF &point);
    void handleRelease(const QPointF &point);
    void handleUngrab();

    qreal from;
    qreal to;
    qreal value;
    qreal position;
    qreal stepSize;
    bool live;
    bool pressed;
    int touchId;
    QPointF pressPoint;
    Qt::Orientation orientation;
    QQuickSlider::SnapMode snapMode;
    QQuickItem *handle;
};

qreal QQuickSliderPrivate::snapPosition(qreal position) const
{
    const qreal range = to - from;
    if (qFuzzyIsNull(range))
        return position;

    const qreal effectiveStep = stepSize / range;
    if (qFuzzyIsNull(effectiveStep))
        return position;

    return qRound(position / effectiveStep) * effectiveStep;
}

qreal QQuickSliderPrivate::positionAt(const QPointF &point) const
{
    Q_Q(const QQuickSlider);
    qreal pos = 0.0;
    if (orientation == Qt::Horizontal) {
        const qreal hw = handle ? handle->width() : 0;
        const qreal offset = hw / 2;
        const qreal extent = q->availableWidth() - hw;
        if (!qFuzzyIsNull(extent)) {
            if (q->isMirrored())
                pos = (q->width() - point.x() - q->rightPadding() - offset) / extent;
            else
                pos = (point.x() - q->leftPadding() - offset) / extent;
        }
    } else {
        const qreal hh = handle ? handle->height() : 0;
        const qreal offset = hh / 2;
        const qreal extent = q->availableHeight() - hh;
        if (!qFuzzyIsNull(extent))
            pos = (q->height() - point.y() - q->bottomPadding() - offset) / extent;
    }
    return qBound<qreal>(0.0, pos, 1.0);
}

void QQuickSliderPrivate::setPosition(qreal pos)
{
    Q_Q(QQuickSlider);
    pos = qBound<qreal>(0.0, pos, 1.0);
    if (qFuzzyCompare(position, pos))
        return;

    position = pos;
    emit q->positionChanged();
    emit q->visualPositionChanged();
}

void QQuickSliderPrivate::updatePosition()
{
    qreal pos = 0;
    if (!qFuzzyCompare(from, to))
        pos = (value - from) / (to - from);
    setPosition(pos);
}

void QQuickSliderPrivate::handlePress(const QPointF &point)
{
    Q_Q(QQuickSlider);
    pressPoint = point;
    q->setPressed(true);
}

void QQuickSliderPrivate::handleMove(const QPointF &point)
{
    Q_Q(QQuickSlider);
    const qreal oldPos = position;
    qreal pos = positionAt(point);
    if (snapMode == QQuickSlider::SnapAlways)
        pos = snapPosition(pos);
    if (live)
        q->setValue(q->valueAt(pos));
    else
        setPosition(pos);
    if (!qFuzzyCompare(pos, oldPos))
        emit q->moved();
}

void QQuickSliderPrivate::handleRelease(const QPointF &point)
{
    Q_Q(QQuickSlider);
    touchId = -1;
    pressPoint = QPointF();
    const qreal oldPos = position;
    qreal pos = positionAt(point);
    if (snapMode != QQuickSlider::NoSnap)
        pos = snapPosition(pos);
    qreal val = q->valueAt(pos);
    if (!qFuzzyCompare(val, value))
        q->setValue(val);
    else if (snapMode != QQuickSlider::NoSnap)
        setPosition(pos);
    if (!qFuzzyCompare(pos, oldPos))
        emit q->moved();
    q->setKeepMouseGrab(false);
    q->setKeepTouchGrab(false);
    q->setPressed(false);
}

void QQuickSliderPrivate::handleUngrab()
{
    Q_Q(QQuickSlider);
    touchId = -1;
    pressPoint = QPointF();
    q->setPressed(false);
}

QQuickSlider::QQuickSlider(QQuickItem *parent)
    : QQuickControl(*(new QQuickSliderPrivate), parent)
{
    setActiveFocusOnTab(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif
}

/*!
    \qmlproperty real QtQuick.Controls::Slider::from

    This property holds the starting value for the range. The default value is \c 0.0.

    \sa to, value
*/
qreal QQuickSlider::from() const
{
    Q_D(const QQuickSlider);
    return d->from;
}

void QQuickSlider::setFrom(qreal from)
{
    Q_D(QQuickSlider);
    if (qFuzzyCompare(d->from, from))
        return;

    d->from = from;
    emit fromChanged();
    if (isComponentComplete()) {
        setValue(d->value);
        d->updatePosition();
    }
}

/*!
    \qmlproperty real QtQuick.Controls::Slider::to

    This property holds the end value for the range. The default value is \c 1.0.

    \sa from, value
*/
qreal QQuickSlider::to() const
{
    Q_D(const QQuickSlider);
    return d->to;
}

void QQuickSlider::setTo(qreal to)
{
    Q_D(QQuickSlider);
    if (qFuzzyCompare(d->to, to))
        return;

    d->to = to;
    emit toChanged();
    if (isComponentComplete()) {
        setValue(d->value);
        d->updatePosition();
    }
}

/*!
    \qmlproperty real QtQuick.Controls::Slider::value

    This property holds the value in the range \c from - \c to. The default value is \c 0.0.

    \sa position
*/
qreal QQuickSlider::value() const
{
    Q_D(const QQuickSlider);
    return d->value;
}

void QQuickSlider::setValue(qreal value)
{
    Q_D(QQuickSlider);
    if (isComponentComplete())
        value = d->from > d->to ? qBound(d->to, value, d->from) : qBound(d->from, value, d->to);

    if (qFuzzyCompare(d->value, value))
        return;

    d->value = value;
    d->updatePosition();
    emit valueChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Slider::position
    \readonly

    This property holds the logical position of the handle.

    The position is expressed as a fraction of the control's size, in the range
    \c {0.0 - 1.0}. For visualizing a slider, the right-to-left aware
    \l visualPosition should be used instead.

    \sa value, visualPosition, valueAt()
*/
qreal QQuickSlider::position() const
{
    Q_D(const QQuickSlider);
    return d->position;
}

/*!
    \qmlproperty real QtQuick.Controls::Slider::visualPosition
    \readonly

    This property holds the visual position of the handle.

    The position is expressed as a fraction of the control's size, in the range
    \c {0.0 - 1.0}. When the control is \l {Control::mirrored}{mirrored}, the
    value is equal to \c {1.0 - position}. This makes the value suitable for
    visualizing the slider, taking right-to-left support into account.

    \sa position
*/
qreal QQuickSlider::visualPosition() const
{
    Q_D(const QQuickSlider);
    if (d->orientation == Qt::Vertical || isMirrored())
        return 1.0 - d->position;
    return d->position;
}

/*!
    \qmlproperty real QtQuick.Controls::Slider::stepSize

    This property holds the step size. The default value is \c 0.0.

    \sa snapMode, increase(), decrease()
*/
qreal QQuickSlider::stepSize() const
{
    Q_D(const QQuickSlider);
    return d->stepSize;
}

void QQuickSlider::setStepSize(qreal step)
{
    Q_D(QQuickSlider);
    if (qFuzzyCompare(d->stepSize, step))
        return;

    d->stepSize = step;
    emit stepSizeChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Slider::snapMode

    This property holds the snap mode.

    Possible values:
    \value Slider.NoSnap The slider does not snap (default).
    \value Slider.SnapAlways The slider snaps while the handle is dragged.
    \value Slider.SnapOnRelease The slider does not snap while being dragged, but only after the handle is released.

    In the following table, the various modes are illustrated with animations.
    The movement of the mouse cursor and the \l stepSize (\c 0.2) are identical
    in each animation.

    \table
    \header
        \row \li \b Value \li \b Example
        \row \li \c Slider.NoSnap \li \image qtquickcontrols2-slider-nosnap.gif
        \row \li \c Slider.SnapAlways \li \image qtquickcontrols2-slider-snapalways.gif
        \row \li \c Slider.SnapOnRelease \li \image qtquickcontrols2-slider-snaponrelease.gif
    \endtable

    \sa stepSize
*/
QQuickSlider::SnapMode QQuickSlider::snapMode() const
{
    Q_D(const QQuickSlider);
    return d->snapMode;
}

void QQuickSlider::setSnapMode(SnapMode mode)
{
    Q_D(QQuickSlider);
    if (d->snapMode == mode)
        return;

    d->snapMode = mode;
    emit snapModeChanged();
}

/*!
    \since QtQuick.Controls 2.2
    \qmlproperty bool QtQuick.Controls::Slider::live

    This property holds whether the slider provides live updates for the \l value
    property while the handle is dragged.

    The default value is \c true.

    \sa value, valueAt()
*/
bool QQuickSlider::live() const
{
    Q_D(const QQuickSlider);
    return d->live;
}

void QQuickSlider::setLive(bool live)
{
    Q_D(QQuickSlider);
    if (d->live == live)
        return;

    d->live = live;
    emit liveChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Slider::pressed

    This property holds whether the slider is pressed.
*/
bool QQuickSlider::isPressed() const
{
    Q_D(const QQuickSlider);
    return d->pressed;
}

void QQuickSlider::setPressed(bool pressed)
{
    Q_D(QQuickSlider);
    if (d->pressed == pressed)
        return;

    d->pressed = pressed;
    setAccessibleProperty("pressed", pressed);
    emit pressedChanged();
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Slider::orientation

    This property holds the orientation.

    Possible values:
    \value Qt.Horizontal Horizontal (default)
    \value Qt.Vertical Vertical
*/
Qt::Orientation QQuickSlider::orientation() const
{
    Q_D(const QQuickSlider);
    return d->orientation;
}

void QQuickSlider::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickSlider);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    emit orientationChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::Slider::handle

    This property holds the handle item.

    \sa {Customizing Slider}
*/
QQuickItem *QQuickSlider::handle() const
{
    Q_D(const QQuickSlider);
    return d->handle;
}

void QQuickSlider::setHandle(QQuickItem *handle)
{
    Q_D(QQuickSlider);
    if (d->handle == handle)
        return;

    QQuickControlPrivate::destroyDelegate(d->handle, this);
    d->handle = handle;
    if (handle && !handle->parentItem())
        handle->setParentItem(this);
    emit handleChanged();
}

/*!
    \since QtQuick.Controls 2.1
    \qmlmethod real QtQuick.Controls::Slider::valueAt(real position)

    Returns the value for the given \a position.

    \sa value, position
*/
qreal QQuickSlider::valueAt(qreal position) const
{
    Q_D(const QQuickSlider);
    return d->from + (d->to - d->from) * position;
}

/*!
    \qmlmethod void QtQuick.Controls::Slider::increase()

    Increases the value by \l stepSize or \c 0.1 if stepSize is not defined.

    \sa stepSize
*/
void QQuickSlider::increase()
{
    Q_D(QQuickSlider);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value + step);
}

/*!
    \qmlmethod void QtQuick.Controls::Slider::decrease()

    Decreases the value by \l stepSize or \c 0.1 if stepSize is not defined.

    \sa stepSize
*/
void QQuickSlider::decrease()
{
    Q_D(QQuickSlider);
    qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
    setValue(d->value - step);
}

void QQuickSlider::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::keyPressEvent(event);

    const qreal oldValue = d->value;
    if (d->orientation == Qt::Horizontal) {
        if (event->key() == Qt::Key_Left) {
            setPressed(true);
            if (isMirrored())
                increase();
            else
                decrease();
            event->accept();
        } else if (event->key() == Qt::Key_Right) {
            setPressed(true);
            if (isMirrored())
                decrease();
            else
                increase();
            event->accept();
        }
    } else {
        if (event->key() == Qt::Key_Up) {
            setPressed(true);
            increase();
            event->accept();
        } else if (event->key() == Qt::Key_Down) {
            setPressed(true);
            decrease();
            event->accept();
        }
    }
    if (!qFuzzyCompare(d->value, oldValue))
        emit moved();
}

void QQuickSlider::keyReleaseEvent(QKeyEvent *event)
{
    QQuickControl::keyReleaseEvent(event);
    setPressed(false);
}

void QQuickSlider::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::mousePressEvent(event);
    d->handlePress(event->localPos());
    d->handleMove(event->localPos());
}

void QQuickSlider::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::mouseMoveEvent(event);
    if (!keepMouseGrab()) {
        if (d->orientation == Qt::Horizontal)
            setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(event->localPos().x() - d->pressPoint.x(), Qt::XAxis, event));
        else
            setKeepMouseGrab(QQuickWindowPrivate::dragOverThreshold(event->localPos().y() - d->pressPoint.y(), Qt::YAxis, event));
    }
    d->handleMove(event->localPos());
}

void QQuickSlider::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::mouseReleaseEvent(event);
    d->handleRelease(event->localPos());
}

void QQuickSlider::mouseUngrabEvent()
{
    Q_D(QQuickSlider);
    QQuickControl::mouseUngrabEvent();
    d->handleUngrab();
}

void QQuickSlider::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickSlider);
    switch (event->type()) {
    case QEvent::TouchBegin:
        if (d->touchId == -1) {
            const QTouchEvent::TouchPoint point = event->touchPoints().first();
            d->touchId = point.id();
            d->handlePress(point.pos());
        } else {
            event->ignore();
        }
        break;

    case QEvent::TouchUpdate:
        for (const QTouchEvent::TouchPoint &point : event->touchPoints()) {
            if (point.id() != d->touchId)
                continue;

            if (!keepTouchGrab()) {
                if (d->orientation == Qt::Horizontal)
                    setKeepTouchGrab(QQuickWindowPrivate::dragOverThreshold(point.pos().x() - d->pressPoint.x(), Qt::XAxis, &point));
                else
                    setKeepTouchGrab(QQuickWindowPrivate::dragOverThreshold(point.pos().y() - d->pressPoint.y(), Qt::YAxis, &point));
            }
            if (keepTouchGrab())
                d->handleMove(point.pos());
        }
        break;

    case QEvent::TouchEnd:
        for (const QTouchEvent::TouchPoint &point : event->touchPoints()) {
            if (point.id() != d->touchId)
                continue;

            d->handleRelease(point.pos());
        }
        break;

    case QEvent::TouchCancel:
        d->handleUngrab();
        break;

    default:
        QQuickControl::touchEvent(event);
        break;
    }
}

void QQuickSlider::touchUngrabEvent()
{
    Q_D(QQuickSlider);
    QQuickControl::touchUngrabEvent();
    d->handleUngrab();
}

#if QT_CONFIG(wheelevent)
void QQuickSlider::wheelEvent(QWheelEvent *event)
{
    Q_D(QQuickSlider);
    QQuickControl::wheelEvent(event);
    if (d->wheelEnabled) {
        const qreal oldValue = d->value;
        const QPointF angle = event->angleDelta();
        const qreal delta = (qFuzzyIsNull(angle.y()) ? angle.x() : (event->inverted() ? -angle.y() : angle.y())) / QWheelEvent::DefaultDeltasPerStep;
        const qreal step = qFuzzyIsNull(d->stepSize) ? 0.1 : d->stepSize;
        setValue(oldValue + step * delta);
        const bool wasMoved = !qFuzzyCompare(d->value, oldValue);
        if (wasMoved)
            emit moved();
        event->setAccepted(wasMoved);
    }
}
#endif

void QQuickSlider::mirrorChange()
{
    QQuickControl::mirrorChange();
    emit visualPositionChanged();
}

void QQuickSlider::componentComplete()
{
    Q_D(QQuickSlider);
    QQuickControl::componentComplete();
    setValue(d->value);
    d->updatePosition();
}

#if QT_CONFIG(accessibility)
void QQuickSlider::accessibilityActiveChanged(bool active)
{
    QQuickControl::accessibilityActiveChanged(active);

    Q_D(QQuickSlider);
    if (active)
        setAccessibleProperty("pressed", d->pressed);
}

QAccessible::Role QQuickSlider::accessibleRole() const
{
    return QAccessible::Slider;
}
#endif

QT_END_NAMESPACE
