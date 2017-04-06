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

#ifndef XYSCENE_H
#define XYSCENE_H

#include <QPen>
#include <QBrush>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QTimer>
#include <cmath>

class XYFunction;
class XYPointList;
struct XYLook
{
    QPen axesPen;
    QPen subaxesPen;
    QColor textColor;
    QPen zoomPen;
    QBrush backgroundBrush;
};

class RealZoom
{
public:
	RealZoom();
	RealZoom(qreal xMin, qreal xMax, qreal yMin, qreal yMax);
	inline qreal xMin() const;
	inline qreal xMax() const;
	inline qreal yMin() const;
	inline qreal yMax() const;
	inline qreal width() const;
	inline qreal height() const;

	inline void setXMin(qreal);
	inline void setXMax(qreal);
	inline void setYMin(qreal);
	inline void setYMax(qreal);

	inline bool isInvalid() const;

private:
	qreal _xmin;
	qreal _xmax;
	qreal _ymin;
	qreal _ymax;
	qreal _width;
	qreal _height;
};

class XYScene : public QGraphicsScene {
    Q_OBJECT
public:
    enum Status {
        //                                 Par defaut
        SendMouseMove         = 0x0001, //             Envoie les coordonées souris par le signal 'coordonneesSouris(QPointF)'
        RegraphOnResize       = 0x0002, //     x       Redessine quand il recoit le signal 'sceneRectChanged(QRectF)'
        SendZoomChanged       = 0x0004, //             Envoie un signal
        AutoZoomOnDoubleClick = 0x0008, //     x       Rezoom sur les nuages de points quand on double clique sur le graph
        ShowPointPosition     = 0x0010  //             Affiche la position des point quand on passe la souris dessus
    };

    explicit XYScene(QObject *parent = 0);
    virtual ~XYScene();

    QList<XYFunction *> &getFunctionsList();
    void addFunction(XYFunction *);
	QList<const XYPointList *> &getPointListList();
	void addScatterplot(const XYPointList *);

    const RealZoom &zoom() const;
    void setZoom(const RealZoom &zoom);
    void setZoom(qreal xmin, qreal xmax, qreal ymin, qreal ymax);
    void setXMin(qreal xmin);
    void setXMax(qreal xmax);
    void setYMin(qreal ymin);
    void setYMax(qreal ymax);

    void relativeZoom(qreal k); // si k < 1.0 ca zoom sinon ca dezoom
    void autoZoom(); // zoom automatique autour des XYScatterplot
    void focusOn(qreal x, qreal y);

    int state() const;
    void setState(int);
    void setFlag(enum Status flag, bool enable = true);

    void setAxesPen(const QPen &pen);
    void setSubaxesPen(const QPen &pen);
    void setTextColor(const QColor &color);
    void setZoomPen(const QPen &pen);
    void setLook(const struct XYLook &look);
    struct XYLook look() const;

    inline qreal xr2i(qreal xr) const;
    inline qreal xi2r(qreal xi) const;
    inline qreal yr2i(qreal yr) const;
    inline qreal yi2r(qreal yi) const;
    inline qreal wr2i(qreal wr) const;
    inline qreal wi2r(qreal wi) const;
    inline qreal hr2i(qreal hr) const;
    inline qreal hi2r(qreal hi) const;
    inline QPointF real2image(const QPointF &p) const;
    inline QPointF image2real(const QPointF &p) const;
    inline QRectF real2image(const RealZoom &r) const;
    inline RealZoom image2real(const QRectF &r) const;

signals:
    void mousePosition(QPointF rpos);
    void mouseClic(QPointF rpos, Qt::MouseButtons = 0);
    void zoomChanged();

public slots:
    void regraph();

private slots:
    void maiRegraph();

private:
    void drawaxes();
    void drawfunctions();
    void drawpoints();

    void keyPressEvent(QKeyEvent *keyEvent);
    void wheelEvent(QGraphicsSceneWheelEvent *wheelEvent);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent);

    enum PointData {
        XValue,
        YValue,
        Name,
        Type,
		TypeScatter
    };

    QTimer *m_timerRegraph;
    QTimer *m_timerZoom;

    QPointF m_zoomRectOrigin;
    QGraphicsRectItem *m_zoomRect;

    QGraphicsTextItem *m_positionPointText;
    QGraphicsEllipseItem *m_positionPointEllipse;

    QPen m_axesPen;
    QPen m_subaxesPen;
    QColor m_textColor;
    QPen m_zoomPen;

    RealZoom m_realSceneRect;

    QList<XYFunction *> m_functions;
	QList<const XYPointList *> m_pointlists;

    int m_state;
    Qt::MouseButtons m_mouseDontMove;
};

class XYFunction
{
public:
    XYFunction(const QPen &pen = QPen());
    XYFunction(const XYFunction &other);
    virtual ~XYFunction() {}
    void setVisible(bool visible);
    bool isVisible() const;
    void setPen(const QPen &pen);
    virtual qreal y(qreal x) = 0;
    virtual bool domain(qreal x) const;

private:
    bool m_visible;
    QPen m_pen;

    friend class XYScene;
};

class XYPointList : public QList<QPointF>
{
public:
	XYPointList(const QPen &dotPen = QPen(), const QBrush &dotBrush = QBrush(), qreal dotRadius = 2.0, const QPen &linePen = QPen(Qt::NoPen));
	XYPointList(const QList<QPointF> &points, const QPen &dotPen = QPen(), const QBrush &dotBrush = QBrush(), qreal dotRadius = 2.0, const QPen &linePen = QPen(Qt::NoPen));
    QList<QPointF> &getPoints();
    const QList<QPointF> &getPoints() const;
    void setVisible(bool visible);
    bool isVisible() const;
    void setPen(const QPen &pen);
    void setBrush(const QBrush &brush);
    void setRadius(qreal r);
    void setLinePen(const QPen &pen);

private:
    bool m_visible;
    QPen m_pen;
    QBrush m_brush;
    qreal m_r;
    QPen m_linepen;

    friend class XYScene;
};


/* INLINE IMPLANTATION */

qreal XYScene::xr2i(qreal xr) const {
    return (xr - m_realSceneRect.xMin()) / m_realSceneRect.width() * (sceneRect().width() - 1) + sceneRect().left();
}

qreal XYScene::xi2r(qreal xi) const {
    return (xi - sceneRect().left()) / (sceneRect().width() - 1) * m_realSceneRect.width() + m_realSceneRect.xMin();
}

qreal XYScene::yr2i(qreal yr) const {
    return - (yr - m_realSceneRect.yMax()) / m_realSceneRect.height() * (sceneRect().height() - 1) + sceneRect().top();
}

qreal XYScene::yi2r(qreal yi) const {
    return - (yi - sceneRect().top()) / (sceneRect().height() - 1) * m_realSceneRect.height() + m_realSceneRect.yMax();
}

qreal XYScene::wr2i(qreal wr) const {
    return wr / m_realSceneRect.width() * (sceneRect().width() - 1);
}

qreal XYScene::wi2r(qreal wi) const {
    return wi / (sceneRect().width() - 1) * m_realSceneRect.width();
}

qreal XYScene::hr2i(qreal hr) const {
    return hr / m_realSceneRect.height() * (sceneRect().height() - 1);
}

qreal XYScene::hi2r(qreal hi) const {
    return hi / (sceneRect().height() - 1) * m_realSceneRect.height();
}

QPointF XYScene::real2image(const QPointF &real) const {
    return QPointF(xr2i(real.x()), yr2i(real.y()));
}

QPointF XYScene::image2real(const QPointF &image) const {
    return QPointF(xi2r(image.x()), yi2r(image.y()));
}

QRectF XYScene::real2image(const RealZoom &real) const {
    return QRectF(QPointF(xr2i(real.xMin()), yr2i(real.yMax())), QPointF(xr2i(real.xMax()), yr2i(real.yMin())));
}

RealZoom XYScene::image2real(const QRectF &image) const {
    return RealZoom(xi2r(image.left()), xi2r(image.right()), yi2r(image.bottom()), yi2r(image.top()));
}




inline RealZoom::RealZoom()
	: _xmin(-10.0), _xmax(10.0), _ymin(-10.0), _ymax(10.0)
{
	_width = _xmax - _xmin;
	_height = _ymax - _ymin;
}

inline RealZoom::RealZoom(qreal xMin, qreal xMax, qreal yMin, qreal yMax)
	: _xmin(xMin), _xmax(xMax), _ymin(yMin), _ymax(yMax)
{
	_width = _xmax - _xmin;
	_height = _ymax - _ymin;
}

inline qreal RealZoom::xMin() const {
	return _xmin;
}

inline qreal RealZoom::xMax() const {
	return _xmax;
}

inline qreal RealZoom::yMin() const {
	return _ymin;
}

inline qreal RealZoom::yMax() const {
	return _ymax;
}

inline qreal RealZoom::width() const {
	return _width;
}

inline qreal RealZoom::height() const {
	return _height;
}

inline void RealZoom::setXMin(qreal x) {
	_xmin = x;
	_width = _xmax - _xmin;
}

inline void RealZoom::setXMax(qreal x) {
	_xmax = x;
	_width = _xmax - _xmin;
}

inline void RealZoom::setYMin(qreal y) {
	_ymin = y;
	_height = _ymax - _ymin;
}

inline void RealZoom::setYMax(qreal y) {
	_ymax = y;
	_height = _ymax - _ymin;
}

inline bool RealZoom::isInvalid() const {
	return _width <= 0.0 || _height <= 0.0 || std::isinf(_width) || std::isinf(_height) || std::isnan(_width) || std::isnan(_height);
}

#endif // XYSCENE_H
