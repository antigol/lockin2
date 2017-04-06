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

#include "xyscene.hh"

#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsSceneMouseEvent>
#include <QFontMetrics>
#include <QTime>
#include <QVector2D>
#include <QDebug>

#include <cmath>

XYScene::XYScene(QObject *parent) :
    QGraphicsScene(parent),
    m_zoomRect(0),
    m_positionPointText(0),
    m_positionPointEllipse(0),
    m_axesPen(Qt::white),
    m_subaxesPen(Qt::gray),
    m_textColor(Qt::white),
    m_zoomPen(Qt::yellow),
    m_state(RegraphOnResize | AutoZoomOnDoubleClick)
{
    setBackgroundBrush(Qt::darkGray);

    m_timerRegraph = new QTimer(this);
    connect(m_timerRegraph, SIGNAL(timeout()), this, SLOT(regraph()));

    m_timerZoom = new QTimer(this);
    m_timerZoom->setSingleShot(true);
    m_timerZoom->setInterval(100);
    connect(m_timerZoom, SIGNAL(timeout()), this, SIGNAL(zoomChanged()));

    connect(this, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(maiRegraph()));
}

XYScene::~XYScene()
{
}

QList<XYFunction *> &XYScene::getFunctionsList()
{
    return m_functions;
}

void XYScene::addFunction(XYFunction *ptr)
{
    m_functions.append(ptr);
}

QList<const XYPointList *> &XYScene::getPointListList()
{
	return m_pointlists;
}

void XYScene::addScatterplot(const XYPointList *ptr)
{
	m_pointlists.append(ptr);
}

void XYScene::maiRegraph()
{
    if ((m_state & XYScene::RegraphOnResize) == XYScene::RegraphOnResize)
        regraph();
}

void XYScene::regraph()
{
    if (m_zoomRect)
        return;

    if (sceneRect().isNull())
        return;

    if (m_realSceneRect.isInvalid())
        return;

    // il va se faire supprimer par clear
    m_positionPointText = 0;
    m_positionPointEllipse = 0;

    clear();

    //    QTime ch;
    //    ch.start();
    drawaxes();
    //    qDebug("drawaxes      %d", ch.restart());
    drawfunctions();
    //    qDebug("drawfunctions %d", ch.restart());
    drawpoints();
    //    qDebug("drawpoints    %d", ch.restart());
}

#define PIXELMIN 40.0
#define LENGTH 5.0
#define SUITESIZE 4
qreal suite[SUITESIZE] = {2.0, 2.5, 5.0, 10.0};

void XYScene::drawaxes()
{
    //	QTime ch;
    //	ch.start();

    QPointF imageZero(real2image(QPointF(0.0, 0.0)));
    if (imageZero.x() < sceneRect().left())
        imageZero.rx() = sceneRect().left();

    if (imageZero.x() > sceneRect().right())
        imageZero.rx() = sceneRect().right();

    if (imageZero.y() < sceneRect().top())
        imageZero.ry() = sceneRect().top();

    if (imageZero.y() > sceneRect().bottom())
        imageZero.ry() = sceneRect().bottom();

    // les deux lignes du 0
    addLine(sceneRect().left(), imageZero.y(), sceneRect().right(), imageZero.y(), m_axesPen);
    addLine(imageZero.x(), sceneRect().top(), imageZero.x(), sceneRect().bottom(), m_axesPen);


    QPainterPath path, subpath;
    qreal div;
    qreal decade;

    //	qDebug("init %d", ch.restart());

    // graduation de l'axe X
    {
        // Recherche de la division
        div = wi2r(PIXELMIN);
        decade = 1.0;
        while (div >= 10.0) {
            div /= 10.0;
            decade *= 10.0;
        }
        while (div < 1.0) {
            div *= 10.0;
            decade /= 10.0;
        }
        for (int i = 0; i < SUITESIZE; ++i) {
            if (div < suite[i]) {
                div = suite[i] * decade;
                break;
            }
        }

        // graduation X avec la division
        qreal yTxtPos = imageZero.y();
        if (imageZero.y() < (sceneRect().top() + sceneRect().bottom()) / 2.0)
            yTxtPos += 5.0;
        else
            yTxtPos -= 27.0;

        const int xmax = ceil(m_realSceneRect.xMax() / div);
        for (int x = ceil(m_realSceneRect.xMin() / div); x < xmax; ++x) {
            if (x == 0)
                continue;
            qreal xreal = x * div;
            qreal ximage = xr2i(xreal);

            path.moveTo(ximage, imageZero.y() - LENGTH);
            path.lineTo(ximage, imageZero.y() + LENGTH);

            subpath.moveTo(ximage, sceneRect().bottom());
            subpath.lineTo(ximage, sceneRect().top());

            if (qAbs(ximage - imageZero.x()) > 30.0) {
                QGraphicsTextItem *txt = addText(QString::number(xreal));
                txt->setZValue(2);
                txt->setDefaultTextColor(m_textColor);
                static QFontMetrics fm(txt->font());

                txt->setPos(ximage - fm.boundingRect(txt->toPlainText()).width() * 0.5 - 2.0,
                            yTxtPos);
            }
        }
    }

    //	qDebug("x %d", ch.restart());

    // graduation de l'axe Y
    {
        // Recherche de la division
        div = hi2r(PIXELMIN);
        decade = 1.0;
        while (div >= 10.0) {
            div /= 10.0;
            decade *= 10.0;
        }
        while (div < 1.0) {
            div *= 10.0;
            decade /= 10.0;
        }
        for (int i = 0; i < SUITESIZE; ++i) {
            if (div < suite[i]) {
                div = suite[i] * decade;
                break;
            }
        }

        // graduation Y avec la division
        bool textOnRight = imageZero.x() < (sceneRect().left() + sceneRect().right()) / 2.0;

        const int ymax = ceil(m_realSceneRect.yMax() / div);
        for (int y = ceil(m_realSceneRect.yMin() / div); y < ymax; ++y) {
            if (y == 0)
                continue;
            qreal yreal = y * div;
            qreal yimage = yr2i(yreal);

            path.moveTo(imageZero.x() - LENGTH, yimage);
            path.lineTo(imageZero.x() + LENGTH, yimage);

            subpath.moveTo(sceneRect().left(), yimage);
            subpath.lineTo(sceneRect().right(), yimage);

            if (qAbs(yimage - imageZero.y()) > 30.0) {
                QGraphicsTextItem *txt = addText(QString::number(yreal));
                txt->setZValue(2);
                txt->setDefaultTextColor(m_textColor);
                static QFontMetrics fm(txt->font());

                QPointF txtpos(imageZero.x(), yimage - fm.height() * 0.72);

                if (textOnRight)
                    txtpos.rx() += 7.0;
                else
                    txtpos.rx() -= fm.boundingRect(txt->toPlainText()).width() + 12.0;

                txt->setPos(txtpos);
            }
        }
    }

    //	qDebug("y %d", ch.restart());

    addPath(path, m_axesPen)->setZValue(1);
    addPath(subpath, m_subaxesPen);

    //	qDebug("end %d", ch.restart());
}

void XYScene::drawfunctions()
{
    bool startPath;
    qreal xreal, yreal;
    QPointF point;

    for (int i = 0; i < m_functions.size(); ++i) {
        if (!m_functions[i]->isVisible())
            continue;

        QPainterPath path;
        startPath = false;
        for (qreal ximage = sceneRect().left(); ximage < sceneRect().right(); ximage += 1.5) {
            xreal = xi2r(ximage);
            point.setX(ximage);
            if (m_functions[i]->domain(xreal)) {
                yreal = m_functions[i]->y(xreal);
                if (!std::isnan(yreal)) {
                    point.setY(yr2i(yreal));
                    if (startPath) {
                        path.lineTo(point);
                    } else {
                        path.moveTo(point);
                        startPath = true;
                    }
                } else {
                    startPath = false;
                }
            } else {
                startPath = false;
            }
        }
        addPath(path, m_functions[i]->m_pen, QBrush(Qt::NoBrush));
    }
}

void XYScene::drawpoints()
{
	for (int i = 0; i < m_pointlists.size(); ++i) {
		if (!m_pointlists[i]->isVisible())
            continue;

		if (m_pointlists[i]->m_linepen != QPen(Qt::NoPen)) {
            QPainterPath path;
			for (int p = 0; p < m_pointlists[i]->size(); ++p) {
				const QPointF point = real2image(m_pointlists[i]->at(p));
                if (p == 0)
                    path.moveTo(point);
                else
                    path.lineTo(point);
            }
			addPath(path, m_pointlists[i]->m_linepen);
        }

        /* draw ellipses */
		const qreal r = m_pointlists[i]->m_r;
        const qreal d = 2.0 * r;
        if (r != 0.0) {
			for (int p = 0; p < m_pointlists[i]->size(); ++p) {
				QGraphicsEllipseItem *item = addEllipse(-r, -r, d, d, m_pointlists[i]->m_pen, m_pointlists[i]->m_brush);
				item->setPos(xr2i(m_pointlists[i]->at(p).x()), yr2i(m_pointlists[i]->at(p).y()));
				item->setData(XValue, m_pointlists[i]->at(p).x());
				item->setData(YValue, m_pointlists[i]->at(p).y());
                item->setData(Type, TypeScatter);
            }
        }
    }
}

const RealZoom &XYScene::zoom() const
{
    return m_realSceneRect;
}

void XYScene::setZoom(const RealZoom &zoom)
{
    m_realSceneRect = zoom;
    if ((m_state & SendZoomChanged) == SendZoomChanged)
        m_timerZoom->start();
}

void XYScene::setZoom(qreal xmin, qreal xmax, qreal ymin, qreal ymax)
{
    Q_ASSERT(ymin <= ymax);
    setZoom(RealZoom(xmin, xmax, ymin, ymax));
}

void XYScene::setXMin(qreal xmin)
{
    m_realSceneRect.setXMin(xmin);
    if ((m_state & SendZoomChanged) == SendZoomChanged)
        m_timerZoom->start();
}

void XYScene::setXMax(qreal xmax)
{
    m_realSceneRect.setXMax(xmax);
    if ((m_state & SendZoomChanged) == SendZoomChanged)
        m_timerZoom->start();
}

void XYScene::setYMin(qreal ymin)
{
    m_realSceneRect.setYMin(ymin);
    if ((m_state & SendZoomChanged) == SendZoomChanged)
        m_timerZoom->start();
}

void XYScene::setYMax(qreal ymax)
{
    m_realSceneRect.setYMax(ymax);
    if ((m_state & SendZoomChanged) == SendZoomChanged)
        m_timerZoom->start();
}

void XYScene::relativeZoom(qreal k)
{
    const qreal xadd = m_realSceneRect.width() * (k - 1.0) / 2.0;
    const qreal yadd = m_realSceneRect.height() * (k - 1.0) / 2.0;
    setZoom(m_realSceneRect.xMin() - xadd, m_realSceneRect.xMax() + xadd,
            m_realSceneRect.yMin() - yadd, m_realSceneRect.yMax() + yadd);
}

void XYScene::autoZoom()
{
    QPointF firstPoint;
	for (int i = 0; i < m_pointlists.size(); ++i) {
		if (!m_pointlists[i]->isEmpty()
				&& m_pointlists[i]->isVisible()) {
			firstPoint = m_pointlists[i]->first();
            break;
        }
    }

    qreal xmin = firstPoint.x();
    qreal xmax = xmin;
    qreal ymin = firstPoint.y();
    qreal ymax = ymin;

	for (int i = 0; i < m_pointlists.size(); ++i) {
		for (int j = 1; j < m_pointlists[i]->size(); ++j) {
			if (m_pointlists[i]->at(j).x() < xmin)
				xmin = m_pointlists[i]->at(j).x();
			if (m_pointlists[i]->at(j).x() > xmax)
				xmax = m_pointlists[i]->at(j).x();
			if (m_pointlists[i]->at(j).y() < ymin)
				ymin = m_pointlists[i]->at(j).y();
			if (m_pointlists[i]->at(j).y() > ymax)
				ymax = m_pointlists[i]->at(j).y();
        }
    }

    if (xmin != xmax && ymin != ymax)
        setZoom(xmin, xmax, ymin, ymax);
}

void XYScene::focusOn(qreal x, qreal y)
{
    const qreal w = m_realSceneRect.width() / 2.0;
    const qreal h = m_realSceneRect.height() / 2.0;

    setZoom(x - w, x + w, y - h, y + h);
}

int XYScene::state() const
{
    return m_state;
}

void XYScene::setState(int st)
{
    m_state = st;
}

void XYScene::setFlag(XYScene::Status flag, bool enable)
{
    if (enable) {
        m_state |= flag;
    } else {
        m_state &= ~flag;
    }
}

void XYScene::setAxesPen(const QPen &pen)
{
    m_axesPen = pen;
}

void XYScene::setSubaxesPen(const QPen &pen)
{
    m_subaxesPen = pen;
}

void XYScene::setTextColor(const QColor &color)
{
    m_textColor = color;
}

void XYScene::setZoomPen(const QPen &pen)
{
    m_zoomPen = pen;
}

void XYScene::setLook(const struct XYLook &look)
{
    m_axesPen = look.axesPen;
    m_subaxesPen = look.subaxesPen;
    m_textColor = look.textColor;
    m_zoomPen = look.zoomPen;
    setBackgroundBrush(look.backgroundBrush);
}

struct XYLook XYScene::look() const
{
    struct XYLook look;
    look.axesPen = m_axesPen;
    look.subaxesPen = m_subaxesPen;
    look.textColor = m_textColor;
    look.zoomPen = m_zoomPen;
    look.backgroundBrush = backgroundBrush();
    return look;
}

void XYScene::keyPressEvent(QKeyEvent *keyEvent)
{
    QGraphicsScene::keyPressEvent(keyEvent);
    keyEvent->accept();

    const qreal addw = m_realSceneRect.width() / 10.0;
    const qreal addh = m_realSceneRect.height() / 10.0;

    RealZoom z = zoom();
    switch (keyEvent->key()) {
    case Qt::Key_Left :
        z.setXMax(m_realSceneRect.xMax() - addw);
        z.setXMin(m_realSceneRect.xMin() - addw);
        setZoom(z);
        break;
    case Qt::Key_Right :
        z.setXMax(m_realSceneRect.xMax() + addw);
        z.setXMin(m_realSceneRect.xMin() + addw);
        setZoom(z);
        break;
    case Qt::Key_Up :
        z.setYMax(m_realSceneRect.yMax() + addh);
        z.setYMin(m_realSceneRect.yMin() + addh);
        setZoom(z);
        break;
    case Qt::Key_Down :
        z.setYMax(m_realSceneRect.yMax() - addh);
        z.setYMin(m_realSceneRect.yMin() - addh);
        setZoom(z);
        break;
    case Qt::Key_Minus :
        relativeZoom(1.1);
        break;
    case Qt::Key_Plus :
        relativeZoom(0.9);
        break;
    default:
        keyEvent->ignore();
    }

    if (keyEvent->isAccepted())
        regraph();
}

void XYScene::wheelEvent(QGraphicsSceneWheelEvent *wheelEvent)
{
    QGraphicsScene::wheelEvent(wheelEvent);

    const qreal dxMin = (wheelEvent->scenePos().x() - sceneRect().left()) / (sceneRect().width() - 1);
    const qreal dxMax = 1.0 - dxMin;

    const qreal dyMax = (wheelEvent->scenePos().y() - sceneRect().top()) / (sceneRect().height() - 1);
    const qreal dyMin = 1.0 - dyMax;

    const qreal zoom = - wheelEvent->delta() / 1200.0;

    const qreal xMin = m_realSceneRect.xMin() - m_realSceneRect.width() * zoom * dxMin;
    const qreal xMax = m_realSceneRect.xMax() + m_realSceneRect.width() * zoom * dxMax;
    const qreal yMin = m_realSceneRect.yMin() - m_realSceneRect.height() * zoom * dyMin;
    const qreal yMax = m_realSceneRect.yMax() + m_realSceneRect.height() * zoom * dyMax;

    setZoom(xMin, xMax, yMin, yMax);

    regraph();
}

void XYScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseMoveEvent(mouseEvent);

	if (mouseEvent->buttons() & Qt::LeftButton) {
            QPointF delta = mouseEvent->scenePos() - mouseEvent->lastScenePos();
            delta.rx() *= m_realSceneRect.width() / (sceneRect().width() - 1.0);
            delta.ry() *= m_realSceneRect.height() / (sceneRect().height() - 1.0);
            setZoom(m_realSceneRect.xMin() - delta.x(), m_realSceneRect.xMax() - delta.x(),
                    m_realSceneRect.yMin() + delta.y(), m_realSceneRect.yMax() + delta.y());
    } else if (mouseEvent->buttons() & Qt::RightButton) {
        if (m_zoomRect) {
            QRectF rect(qMin(mouseEvent->scenePos().x(), m_zoomRectOrigin.x()),
                        qMin(mouseEvent->scenePos().y(), m_zoomRectOrigin.y()),
                        qAbs(mouseEvent->scenePos().x() - m_zoomRectOrigin.x()),
                        qAbs(mouseEvent->scenePos().y() - m_zoomRectOrigin.y()));

            m_zoomRect->setRect(rect);
        }
    } else if ((m_state & ShowPointPosition) == ShowPointPosition) {
        QPointF tol(10, 10);
        QRectF area(mouseEvent->scenePos() - tol, mouseEvent->scenePos() + tol);
        QList<QGraphicsItem *> list = items(area, Qt::ContainsItemBoundingRect);

        QGraphicsEllipseItem *item = 0;
        qreal min = 0.0;
        for (int i = 0; i < list.size(); ++i) {
            QGraphicsEllipseItem *currentItem = dynamic_cast<QGraphicsEllipseItem *>(list[i]);
            if (currentItem != 0 && currentItem != m_positionPointEllipse) {
                qreal dist = QVector2D(currentItem->scenePos() - mouseEvent->scenePos()).lengthSquared();
                if (dist < min || item == 0) {
                    min = dist;
                    item = currentItem;
                }
            }
        }
        if (item == 0) {
            if (m_positionPointEllipse != 0)
                m_positionPointEllipse->setVisible(false);

            if (m_positionPointText != 0)
                m_positionPointText->setVisible(false);
        } else {
            qreal d = item->rect().width();
            d = qMax(d * 1.5, d + 4.0);
            QRectF r(0, 0, d, d);
            r.moveCenter(QPointF(0, 0));

            if (m_positionPointEllipse == 0) {
                m_positionPointEllipse = addEllipse(r, m_zoomPen);
            } else {
                m_positionPointEllipse->setRect(r);
                m_positionPointEllipse->setVisible(true);
            }

            m_positionPointEllipse->setPos(item->scenePos());

            m_positionPointEllipse->setData(XValue, item->data(XValue));
            m_positionPointEllipse->setData(XValue, item->data(XValue));
            m_positionPointEllipse->setData(Type, item->data(Type));

            if (m_positionPointText == 0)
                m_positionPointText = addText("");

            m_positionPointText->setVisible(true);
            m_positionPointText->setPlainText(QString("(%1, %2)")
                                              .arg(item->data(XValue).toDouble())
											  .arg(item->data(YValue).toDouble()));
            m_positionPointText->setZValue(2);
            m_positionPointText->setDefaultTextColor(m_textColor);
            static QFontMetrics fm(m_positionPointText->font());

            m_positionPointText->setPos(item->scenePos() + m_positionPointEllipse->rect().topRight() +
                                        QPointF(0.0, -fm.boundingRect(m_positionPointText->toPlainText()).height()));
        }
    }

    if ((m_state & SendMouseMove) == SendMouseMove)
        emit mousePosition(image2real(mouseEvent->scenePos()));

    m_mouseDontMove = Qt::NoButton;
}

void XYScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mousePressEvent(mouseEvent);

	if (mouseEvent->buttons() & Qt::LeftButton) {
        m_timerRegraph->start();
    }

    if (mouseEvent->buttons() & Qt::RightButton) {
        m_zoomRect = addRect(QRectF(mouseEvent->scenePos(), mouseEvent->scenePos()), m_zoomPen);
        m_zoomRectOrigin = mouseEvent->scenePos();
    }

    m_mouseDontMove = mouseEvent->buttons();
}

void XYScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseReleaseEvent(mouseEvent);

    if ((m_state & SendMouseMove) == SendMouseMove && m_mouseDontMove)
        emit mouseClic(image2real(mouseEvent->scenePos()), m_mouseDontMove);

    if (m_zoomRect) {
        if (m_zoomRect->rect().width() > 0.0 && m_zoomRect->rect().height() > 0.0) {
            setZoom(image2real(m_zoomRect->rect()));
        }
        removeItem(m_zoomRect);
        m_zoomRect = 0;
        regraph();
    }

    m_timerRegraph->stop();
}

void XYScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseDoubleClickEvent(mouseEvent);

    if ((mouseEvent->buttons() & Qt::RightButton) && (m_state & AutoZoomOnDoubleClick) == AutoZoomOnDoubleClick) {
        autoZoom();
        regraph();
    }

}

/* XYFunction IMPLANTATION */

XYFunction::XYFunction(const QPen &pen)
    : m_visible(true), m_pen(pen)
{
}

XYFunction::XYFunction(const XYFunction &other)
    : m_visible(true), m_pen(other.m_pen)
{
}

void XYFunction::setVisible(bool visible)
{
    m_visible = visible;
}

bool XYFunction::isVisible() const
{
    return m_visible;
}

void XYFunction::setPen(const QPen &pen)
{
    m_pen = pen;
}

bool XYFunction::domain(qreal x) const
{
    Q_UNUSED(x);
    return true;
}

/* XYScatterplot IMPLANTATION */

XYPointList::XYPointList(const QPen &pen, const QBrush &brush, qreal r, const QPen &linePen)
    : m_visible(true), m_pen(pen), m_brush(brush), m_r(r), m_linepen(linePen)
{
}

XYPointList::XYPointList(const QList<QPointF> &points, const QPen &pen, const QBrush &brush, qreal r, const QPen &linePen)
    : QList<QPointF>(points), m_visible(true), m_pen(pen), m_brush(brush), m_r(r), m_linepen(linePen)
{
}

QList<QPointF> &XYPointList::getPoints()
{
    return *this;
}

const QList<QPointF> &XYPointList::getPoints() const
{
    return *this;
}

void XYPointList::setVisible(bool visible)
{
    m_visible = visible;
}

bool XYPointList::isVisible() const
{
    return m_visible;
}

void XYPointList::setPen(const QPen &pen)
{
    m_pen = pen;
}

void XYPointList::setBrush(const QBrush &brush)
{
    m_brush = brush;
}

void XYPointList::setRadius(qreal r) {
    m_r = r;
}

void XYPointList::setLinePen(const QPen &pen) {
    m_linepen = pen;
}
