//////////////////////////////////////////////////////////////////////
//
// This file is part of BeeBEEP.
//
// BeeBEEP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// BeeBEEP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with BeeBEEP.  If not, see <http://www.gnu.org/licenses/>.
//
// Author: Marco Mastroddi <marco.mastroddi(AT)gmail.com>
//
// $Id: LifeBoard.cpp 346 2015-04-05 16:12:37Z mastroddi $
//
//////////////////////////////////////////////////////////////////////

#include <QKeyEvent>
#include <QPainter>
#include <QtDebug>
#include "LifeBoard.h"
#include "Random.h"


LifeBoard::LifeBoard( QWidget *parent )
 : QFrame( parent )
{
  setFrameStyle( QFrame::NoFrame );
  setFocusPolicy( Qt::StrongFocus );
  setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

  m_isPaused = true;
  Random::init();

  m_stepTimeout = 200;

  bigBang();
}

void LifeBoard::paintEvent( QPaintEvent *event )
{
  QFrame::paintEvent( event );

  QPainter painter( this );
  QRect rect = contentsRect();

  int board_top_x = rect.left() + 1; // border considered
  int board_top_y = rect.top() + 1;

  if( event->rect().isValid() && event->rect().width() <= squareWidth() )
  {
    int x = 0;
    int y = 0;
    if( findCell( &x, &y, QPoint( event->rect().x(), event->rect().y() ) ) )
    {
      drawSquare( painter, board_top_x + x * squareWidth(), board_top_y + y * squareHeight(), m_board[ x ][ y ], m_visited[ x ][ y ] );
      return;
    }
  }

  QRect square_rect;
  for( int i = 0; i < BoardWidth; i++ )
  {
    for( int j = 0; j < BoardHeight; j++ )
    {
      square_rect = drawSquare( painter, board_top_x + i * squareWidth(), board_top_y + j * squareHeight(),
                                m_board[ i ][ j ], m_visited[ i ][ j ] );
      m_rectSpace[ i ][ j ] = square_rect;
    }
  }
}

int LifeBoard::visitedCount() const
{
  int visited_count = 0;
  for( int i = 0; i < BoardWidth; i++ )
  {
    for( int j = 0; j < BoardHeight; j++ )
    {
      if( m_visited[ i ][ j ] )
        visited_count++;
    }
  }
  return visited_count > 0 ? visited_count : 1;
}

int LifeBoard::diedCount() const
{
  int died_count = 0;
  for( int i = 0; i < BoardWidth; i++ )
  {
    for( int j = 0; j < BoardHeight; j++ )
    {
      if( m_visited[ i ][ j ] && !m_board[ i ][ j ])
        died_count++;
    }
  }
  return died_count;
}

int LifeBoard::aliveCount() const
{
  int alive_count = 0;
  for( int i = 0; i < BoardWidth; i++ )
  {
    for( int j = 0; j < BoardHeight; j++ )
    {
      if( m_board[ i ][ j ] )
        alive_count++;
    }
  }
  return alive_count;
}

void LifeBoard::clearBoard()
{
  m_steps = 0;
  m_evolutionCycle = 0;

  for( int i = 0; i < BoardWidth; i++ )
  {
    for( int j = 0; j < BoardHeight; j++ )
    {
      m_board[ i ][ j ] = false;
      m_visited[ i ][ j ] = false;
      m_rectSpace[ i ][ j ] = QRect();
    }
  }
}

void LifeBoard::bigBang()
{
  m_steps = 0;
  m_evolutionCycle = 0;

  for( int i = 0; i < BoardWidth; i++ )
  {
    for( int j = 0; j < BoardHeight; j++ )
    {
      m_board[ i ][ j ] = Random::d100() < 15;
      m_visited[ i ][ j ] = m_board[ i ][ j ];
      m_rectSpace[ i ][ j ] = QRect();
    }
  }
}

void LifeBoard::evolve()
{
  if( isPaused() )
    return;

  int num_neighbors = 0;
  int prev_count = aliveCount();

  bool board_tmp [BoardWidth][BoardHeight];
  int coord_x = 0;
  int coord_y = 0;

  for( int x = 0; x < BoardWidth; x++ )
  {
    for( int y = 0; y < BoardHeight ; y++ )
    {
      for( int x1 = x - 1; x1 <= x + 1; x1++ )
      {
        for( int y1 = y - 1; y1 <= y + 1; y1++ )
        {
          if( x1 < 0 )
            coord_x = BoardWidth - 1;
          else if( x1 >= BoardWidth )
            coord_x = 0;
          else
            coord_x = x1;

          if( y1 < 0 )
            coord_y = BoardHeight - 1;
          else if( y1 >= BoardHeight )
            coord_y = 0;
          else
            coord_y = y1;

          if( m_board[ coord_x ][ coord_y ] )
            num_neighbors++;
        }
      }

      if( m_board[ x ][ y ] )
        num_neighbors--;

      board_tmp[ x ][ y ] = (num_neighbors == 3 || (num_neighbors == 2 && m_board[ x ][ y ] ) );

      if( board_tmp[ x ][ y ] )
        m_visited[ x ][ y ] = true;

      num_neighbors = 0;
    }
  }

  for( int i = 0 ; i < BoardWidth ; i++ )
    for (int j = 0 ; j < BoardHeight ; j++ )
      m_board[i][j] = board_tmp[i][j];

  if( prev_count == aliveCount() )
    m_evolutionCycle++;
  else
    m_evolutionCycle = 0;

  m_steps++;
}

QRect LifeBoard::drawSquare( QPainter& painter, int x, int y, bool is_living, bool is_visited )
{
  QColor color = is_living ? QColor( Qt::yellow ) : ( is_visited ? QColor( Qt::darkGray ) : QColor( Qt::black ) );
  painter.fillRect( x + 1, y + 1, squareWidth() - 2, squareHeight() - 2, color );

  painter.setPen( color.light() );
  painter.drawLine( x, y + squareHeight() - 1, x, y );
  painter.drawLine( x, y, x + squareWidth() - 1, y );

  painter.setPen( color.dark() );
  painter.drawLine( x + 1, y + squareHeight() - 1, x + squareWidth() - 1, y + squareHeight() - 1 );
  painter.drawLine( x + squareWidth() - 1, y + squareHeight() - 1, x + squareWidth() - 1, y + 1 );

  return QRect( x , y , squareWidth(), squareHeight() );
}

bool LifeBoard::findCell( int* x, int* y, const QPoint& pt )
{
  QRect square_rect;
  for( int i = 0; i < BoardWidth; i++ )
  {
    for( int j = 0; j < BoardHeight; j++ )
    {
      square_rect = m_rectSpace[ i ][ j ];
      if( !square_rect.isNull() && square_rect.contains( pt ) )
      {
        *x = i;
        *y = j;
        return true;
      }
    }
  }
  return false;
}

void LifeBoard::keyPressEvent( QKeyEvent* event )
{
  if( event->key() == Qt::Key_Space )
    startOrPause();
  else
    QFrame::keyPressEvent( event );
}

void LifeBoard::mouseReleaseEvent( QMouseEvent* event )
{
  if( event->button() == Qt::LeftButton || event->button() == Qt::RightButton )
  {
    QPoint pt = event->pos();
    int x = 0;
    int y = 0;
    if( findCell( &x, &y, pt ) )
    {
      if( event->button() == Qt::LeftButton )
      {
        m_board[ x ][ y ] = !(m_board[ x ][ y ] );
        if( m_board[ x ][ y ] )
          m_visited[ x ][ y ] = true;
      }
      else
      {
        m_board[ x ][ y ] = false;
        m_visited[ x ][ y ] = false;
      }

      event->accept();
      update( m_rectSpace[ x ][ y ] );
      emit( evolved() );
      return;
    }
  }

  QWidget::mouseReleaseEvent( event );
}

void LifeBoard::pause()
{
  if( isPaused() )
    return;

  startOrPause();
}

void LifeBoard::restart()
{
  pause();
  bigBang();
  emit( evolved() );
}

void LifeBoard::clear()
{
  pause();
  clearBoard();
  emit( evolved() );
}

void LifeBoard::startOrPause()
{
  m_isPaused = !m_isPaused;

  if( isPaused() )
  {
    m_timer.stop();
    update();
    emit( paused() );
  }
  else
  {
    m_timer.start( stepTimeout(), this );
    update();
    emit( running() );
  }
}

void LifeBoard::timerEvent( QTimerEvent* event )
{
  if( event->timerId() == m_timer.timerId() )
  {
    evolve();
    update();
    if( m_evolutionCycle > 5 || aliveCount() == 0 )
    {
      m_isPaused = true;
      m_timer.stop();
      m_evolutionCycle = 0;
      emit( completed() );
    }
    else
      emit( evolved() );
  }
  else
    QFrame::timerEvent(event);
}

QString LifeBoard::status() const
{
  QString status_string = "";

  for( int i = 0 ; i < BoardWidth ; i++ )
  {
    for (int j = 0 ; j < BoardHeight ; j++ )
    {
      if( m_board[ i ][ j ] )
        status_string.append( "1" );
      else if( m_visited[ i ][ j ] )
        status_string.append( "2" );
      else
        status_string.append( "0" );
    }
  }
  return status_string;
}

void LifeBoard::setStatus( int status_steps, const QString& status_string )
{
  clearBoard();

  if( status_string.size() <= 0 )
    return;

  m_steps = status_steps;

  int status_string_index = 0;
  QChar c;

  for( int i = 0 ; i < BoardWidth ; i++ )
  {
    for (int j = 0 ; j < BoardHeight ; j++ )
    {
      if( status_string_index < status_string.size() )
      {
        c = status_string.at( status_string_index );

        if( c == QChar( '1' ) )
        {
          m_board[ i ][ j ] = true;
          m_visited[ i ][ j ] = true;
        }
        else if( c == QChar( '2' ) )
        {
          m_visited[ i ][ j ] = true;
        }
      }
      status_string_index++;
    }
  }

  emit( evolved() );
}
