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

#ifndef XYGRAPH_H
#define XYGRAPH_H

#include <QGraphicsView>

class QResizeEvent;
class QTimer;

class XYView : public QGraphicsView {
    Q_OBJECT
public:
	explicit XYView(QWidget *parent = 0);
	explicit XYView(QGraphicsScene *scene, QWidget *parent = 0);

protected:
    virtual void resizeEvent(QResizeEvent *);

public slots:
    void startRegraphTimer();
    void resizeScene();

private:
    inline void initialisation();
    QTimer *m_timer;
};

#endif // XYGRAPH_H
