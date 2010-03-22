#ifndef SQUAREGRID_H_
#define SQUAREGRID_H_

#include "Grid.h"

class SquareGrid: public Grid
{
public:
  enum ESquareDirection
  {
      ReverseX = 0x1,
      ReverseY = 0x2
  };
  SquareGrid(int size, int direction, QGraphicsItem * parent = 0) :
    Grid(size, parent),
    square_direction(direction)
  {
  }

  virtual QRectF boundingRect() const;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *style, QWidget *widget);

  virtual QList<QPair<int,int> > getHandicapCoordinates() const;

  virtual int minimalXCoordinate(int ) const { return 1; }
  virtual int maximalXCoordinate(int ) const { return m_size; }

  virtual int minimalYCoordinate(int ) const { return 1; }
  virtual int maximalYCoordinate(int ) const { return m_size; }

  virtual int minimalXCoordinate() const { return 1; }
  virtual int maximalXCoordinate() const { return m_size; }

  virtual int minimalYCoordinate() const { return 1; }
  virtual int maximalYCoordinate() const { return m_size; }

  virtual QPainterPath getPath() const;
  virtual Ruler* createRuler();

  virtual QPointF fieldSize() const { return QPointF(s_width, s_height); }

protected:
  virtual QPointF getFieldPosition(int x, int y) const;

  static const qreal s_width, s_height;

  int square_direction;
};

#endif /* SQUAREGRID_H_ */
