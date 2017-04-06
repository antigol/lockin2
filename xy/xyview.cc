/*
		   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
				   Version 2, December 2004

Copyright (C) 2017 Mario Geiger <mario.geiger@epfl.ch>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

		   DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include "xyview.hh"

#include <QResizeEvent>
#include <QTimer>

XYView::XYView(QWidget *parent) :
    QGraphicsView(parent)
{
    initialisation();
}

XYView::XYView(QGraphicsScene *scene, QWidget *parent) :
    QGraphicsView(scene, parent)
{
    initialisation();
}

inline void XYView::initialisation()
{
    setFocusPolicy(Qt::WheelFocus);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(resizeScene()));
}

void XYView::startRegraphTimer()
{
    if (scene()) {
        m_timer->start();
//        scene()->clear();
    }
}

void XYView::resizeEvent(QResizeEvent *e)
{
    startRegraphTimer();

    QGraphicsView::resizeEvent(e);
}

void XYView::resizeScene()
{
    if (scene())
        scene()->setSceneRect(geometry());
}
