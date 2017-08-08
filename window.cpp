/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include <QDebug>

#include <QEvent>
#ifndef QT_NO_GESTURES
#include <QMouseEvent>
#include <QGesture>
#include <QTapAndHoldGesture>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QTabBar>
#include <QTabWidget>
#include <QMdiSubWindow>
#include <QTextEdit>
#include <QScrollBar>
#endif

#include "window.h"

#define TAPANDHOLD_DEBUG

bool KdeMacThemeEventFilter::handleGestureForObject(const QObject *obj) const
#ifndef QT_NO_GESTURES
{
    // this function is called with an <obj> that is or inherits a QWidget
    const QPushButton *btn = qobject_cast<const QPushButton*>(obj);
    const QToolButton *tbtn = qobject_cast<const QToolButton*>(obj);
    if (tbtn) {
        return true /*!tbtn->menu()*/;
    } else if (btn) {
        return true /*!btn->menu()*/;
    } else {
        return (qobject_cast<const QTabBar*>(obj) || qobject_cast<const QTabWidget*>(obj)
//                 || obj->inherits("QTabBar") || obj->inherits("QTabWidget")
            || qobject_cast<const QMdiSubWindow*>(obj)
            || qobject_cast<const QTextEdit*>(obj)
            || qobject_cast<const QScrollBar*>(obj)
            // this catches items in directory lists and the like
            || obj->objectName() == QStringLiteral("qt_scrollarea_viewport")
            || obj->inherits("KateViewInternal"));
        // Konsole windows can be found as obj->inherits("Konsole::TerminalDisplay") but
        // for some reason Konsole doesn't respond to synthetic ContextMenu events
    }
#endif
}

bool KdeMacThemeEventFilter::eventFilter(QObject *obj, QEvent *event)
{
#ifndef QT_NO_GESTURES
    static QVariant qTrue(true), qFalse(false);
// #ifdef TAPANDHOLD_DEBUG
//     if (qEnvironmentVariableIsSet("TAPANDHOLD_CONTEXTMENU_DEBUG")) {
//         QVariant isGrabbed = obj->property("OurTaHGestureActive");
//         if (isGrabbed.isValid() && isGrabbed.toBool()) {
//             qWarning() << "event=" << event << "grabbed obj=" << obj;
//         }
//     }
// #endif
    switch (event->type()) {
        case QEvent::MouseButtonPress: {
            QMouseEvent *me = dynamic_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && me->modifiers() == Qt::NoModifier) {
                QWidget *w = qobject_cast<QWidget*>(obj);
                if (w && handleGestureForObject(obj)) {
                    QVariant isGrabbed = obj->property("OurTaHGestureActive");
                    if (!(isGrabbed.isValid() && isGrabbed.toBool())) {
                        // ideally we'd check first - if we could.
                        // storing all grabbed QObjects is potentially dangerous since we won't
                        // know when they go stale.
                        w->grabGesture(Qt::TapAndHoldGesture);
                        // accept this event but resend it so that the 1st mousepress
                        // can also trigger a tap-and-hold!
                        obj->setProperty("OurTaHGestureActive", qTrue);
#ifdef TAPANDHOLD_DEBUG
                        if (qEnvironmentVariableIsSet("TAPANDHOLD_CONTEXTMENU_DEBUG")) {
                            qWarning() << "event=" << event << "grabbing obj=" << obj << "parent=" << obj->parent();
                        }
#endif
                        if (!m_grabbing.contains(obj)) {
                            QMouseEvent relay(*me);
                            me->accept();
                            m_grabbing.insert(obj);
                            int ret = QCoreApplication::sendEvent(obj, &relay);
                            m_grabbing.remove(obj);
                            return ret;
                        }
                    }
                }
#ifdef TAPANDHOLD_DEBUG
                else if (w && qEnvironmentVariableIsSet("TAPANDHOLD_CONTEXTMENU_DEBUG")) {
                    qWarning() << "event=" << event << "obj=" << obj << "parent=" << obj->parent();
                }
#endif
            }
            // NB: don't "eat" the event if no action was taken!
            break;
        }
//         case QEvent::Paint:
//             if (pressedMouseButtons() == 1) {
//                 // ignore QPaintEvents when the left mouse button (1<<0) is being held
//                 break;
//             } else {
//                 // not holding the left mouse button; fall through to check if
//                 // maybe we should cancel a click-and-hold-opens-contextmenu process.
//             }
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease: {
            QVariant isGrabbed = obj->property("OurTaHGestureActive");
            if (isGrabbed.isValid() && isGrabbed.toBool()) {
#ifdef TAPANDHOLD_DEBUG
                qWarning() << "cancelling TabAndHold because of event=" << event << "obj=" << obj << "parent=" << obj->parent()
                    << "grabbed=" << obj->property("OurTaHGestureActive");
#endif
                obj->setProperty("OurTaHGestureActive", qFalse);
            }
            break;
        }
        case QEvent::Gesture: {
            QGestureEvent *gEvent = static_cast<QGestureEvent*>(event);
            if (QTapAndHoldGesture *heldTap = static_cast<QTapAndHoldGesture*>(gEvent->gesture(Qt::TapAndHoldGesture))) {
                if (heldTap->state() == Qt::GestureFinished) {
                    QVariant isGrabbed = obj->property("OurTaHGestureActive");
                    if (isGrabbed.isValid() && isGrabbed.toBool() /*&& pressedMouseButtons() == 1*/) {
                        QWidget *w = qobject_cast<QWidget*>(obj);
                        // user clicked and held a button, send it a simulated ContextMenuEvent
                        // but send a simulated buttonrelease event first.
                        QPoint localPos = w->mapFromGlobal(heldTap->position().toPoint());
                        QContextMenuEvent ce(QContextMenuEvent::Mouse, localPos, heldTap->hotSpot().toPoint());
                        // don't send a ButtonRelease event to Q*Buttons because we don't want to trigger them
                        if (QPushButton *btn = qobject_cast<QPushButton*>(obj)) {
                            btn->setDown(false);
                            obj->setProperty("OurTaHGestureActive", qFalse);
                        } else if (QToolButton *tbtn = qobject_cast<QToolButton*>(obj)) {
                            tbtn->setDown(false);
                            obj->setProperty("OurTaHGestureActive", qFalse);
                        } else {
                            QMouseEvent me(QEvent::MouseButtonRelease, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
#ifdef TAPANDHOLD_DEBUG
                            qWarning() << "Sending" << &me;
#endif
                            // we'll be unsetting OurTaHGestureActive in the MouseButtonRelease handler above
                            QCoreApplication::sendEvent(obj, &me);
                        }
                        qWarning() << "Sending" << &ce << "to" << obj << "because of" << gEvent << "isGrabbed=" << isGrabbed;
                        bool ret = QCoreApplication::sendEvent(obj, &ce);
                        gEvent->accept();
                        qWarning() << "\tsendEvent" << &ce << "returned" << ret;
                        return true;
                    }
                }
            }
            break;
        }
#ifdef TAPANDHOLD_DEBUG
        case QEvent::ContextMenu:
            if (qEnvironmentVariableIsSet("TAPANDHOLD_CONTEXTMENU_DEBUG")) {
                qWarning() << "event=" << event << "obj=" << obj << "parent=" << obj->parent()
                    << "grabbed=" << obj->property("OurTaHGestureActive");
            }
            break;
#endif
        default:
            break;
    }
#endif
    return false;
}

//! [0]
Window::Window(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(createFirstExclusiveGroup(), 0, 0);
    grid->addWidget(createSecondExclusiveGroup(), 1, 0);
    grid->addWidget(createNonExclusiveGroup(), 0, 1);
    grid->addWidget(createPushButtonGroup(), 1, 1);
    setLayout(grid);

    setWindowTitle(tr("Group Boxes"));
    resize(480, 320);
#ifndef QT_NO_GESTURES
    qApp->installEventFilter(new KdeMacThemeEventFilter(this));
#endif
}
//! [0]

//! [1]
QGroupBox *Window::createFirstExclusiveGroup()
{
//! [2]
    QGroupBox *groupBox = new QGroupBox(tr("Exclusive Radio Buttons"));

    QRadioButton *radio1 = new QRadioButton(tr("&Radio button 1"));
    QRadioButton *radio2 = new QRadioButton(tr("R&adio button 2"));
    QRadioButton *radio3 = new QRadioButton(tr("Ra&dio button 3"));

    radio1->setChecked(true);
//! [1] //! [3]

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(radio1);
    vbox->addWidget(radio2);
    vbox->addWidget(radio3);
    vbox->addStretch(1);
    groupBox->setLayout(vbox);
//! [2]

    return groupBox;
}
//! [3]

//! [4]
QGroupBox *Window::createSecondExclusiveGroup()
{
    QGroupBox *groupBox = new QGroupBox(tr("E&xclusive Radio Buttons"));
    groupBox->setCheckable(true);
    groupBox->setChecked(false);
//! [4]

//! [5]
    QRadioButton *radio1 = new QRadioButton(tr("Rad&io button 1"));
    QRadioButton *radio2 = new QRadioButton(tr("Radi&o button 2"));
    QRadioButton *radio3 = new QRadioButton(tr("Radio &button 3"));
    radio1->setChecked(true);
    QCheckBox *checkBox = new QCheckBox(tr("Ind&ependent checkbox"));
    checkBox->setChecked(true);
//! [5]

//! [6]
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(radio1);
    vbox->addWidget(radio2);
    vbox->addWidget(radio3);
    vbox->addWidget(checkBox);
    vbox->addStretch(1);
    groupBox->setLayout(vbox);

    return groupBox;
}
//! [6]

//! [7]
QGroupBox *Window::createNonExclusiveGroup()
{
    QGroupBox *groupBox = new QGroupBox(tr("Non-Exclusive Checkboxes"));
    groupBox->setFlat(true);
//! [7]

//! [8]
    QCheckBox *checkBox1 = new QCheckBox(tr("&Checkbox 1"));
    QCheckBox *checkBox2 = new QCheckBox(tr("C&heckbox 2"));
    checkBox2->setChecked(true);
    QCheckBox *tristateBox = new QCheckBox(tr("Tri-&state button"));
    tristateBox->setTristate(true);
//! [8]
    tristateBox->setCheckState(Qt::PartiallyChecked);

//! [9]
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(checkBox1);
    vbox->addWidget(checkBox2);
    vbox->addWidget(tristateBox);
    vbox->addStretch(1);
    groupBox->setLayout(vbox);

    return groupBox;
}
//! [9]

//! [10]
QGroupBox *Window::createPushButtonGroup()
{
    QGroupBox *groupBox = new QGroupBox(tr("&Push Buttons"));
    groupBox->setCheckable(true);
    groupBox->setChecked(true);
//! [10]

//! [11]
    QPushButton *pushButton = new QPushButton(tr("&Normal Button"));
    toggleButton = new QPushButton(tr("&Toggle Button"), this);
    toggleButton->setCheckable(true);
    toggleButton->setChecked(true);
    QPushButton *flatButton = new QPushButton(tr("&Flat Button"));
    flatButton->setFlat(true);
//! [11]

//! [12]
    popupButton = new QPushButton(this);
    popupButton->setText(tr("Pop&up Button"));
    QMenu *menu = new QMenu(this);
    menu->addAction(tr("&First Item"));
    menu->addAction(tr("&Second Item"));
    menu->addAction(tr("&Third Item"));
    menu->addAction(tr("F&ourth Item"));
    popupButton->setMenu(menu);
    popupButton->setContextMenuPolicy(Qt::ActionsContextMenu);
//! [12]

    QAction *newAction = menu->addAction(tr("Submenu"));
    QMenu *subMenu = new QMenu(tr("Popup Submenu"));
    subMenu->addAction(tr("Item 1"));
    subMenu->addAction(tr("Item 2"));
    subMenu->addAction(tr("Item 3"));
    newAction->setMenu(subMenu);

    toggleFlat = new QCheckBox(tr("&Flat buttons"));
    toggleFlat->setChecked(false);
    connect(toggleFlat, SIGNAL(stateChanged(int)), this, SLOT(flatButtonToggle(int)));

//! [13]
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(toggleFlat);
    vbox->addWidget(pushButton);
    vbox->addWidget(toggleButton);
    vbox->addWidget(flatButton);
    vbox->addWidget(popupButton);
    vbox->addStretch(1);
    groupBox->setLayout(vbox);

    return groupBox;
}

void Window::flatButtonToggle(int state)
{
    toggleButton->setFlat(state);
    QPushButton *pbtn = qobject_cast<QPushButton*>(popupButton);
    if (pbtn) {
        pbtn->setFlat(state);
    }
}

//! [13]
