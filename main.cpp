// Oleg Kotov

#include <SFML/Graphics.hpp>

#include <vector>
#include <stdlib.h> // random

enum class SelectMode : uint8_t
{
	MouseSelectBox,
	MouseAroundBox,
	MouseAroundCircle
};

int win_width = 800;
int win_height = 800;

SelectMode selectMode = SelectMode::MouseSelectBox;

int spawnCount = 90;

int pointCount = 0;
int checkCount = 0;
int selectedCount = 0;

bool drawQuadTreeQuads = true;
bool drawQuadTreePoints = true;
bool drawSelectedPoints = true;

float random( float min, float max )
{
    return rand() / (float)RAND_MAX * ( max - min ) + min;
}

class Point
{
public:

	Point() = default;

	Point( float x, float y )
	{
		this->x = x;
		this->y = y;
	}

public:

	float x = 0.0f;
	float y = 0.0f;
	void* userData = nullptr;
};

class Circle
{
public:

	Circle() = default;

	Circle( float x, float y, float radius )
	{
		this->x = x;
		this->y = y;
		this->radius = radius;
	}

public:

	bool contains( const Point& point ) const
	{
		float dx = point.x - x;
		float dy = point.y - y;

		float distanceSqr = dx * dx + dy * dy;

		return distanceSqr <= radius * radius;
	}

public:

	float x = 0.0f;
	float y = 0.0f;
	float radius = 0.5f;
};

class Rectangle
{
public:

	Rectangle() = default;

	Rectangle( float x, float y, float width, float height )
	{
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
	}

public:

	bool contains( const Point& point ) const
	{
		return point.x >= x && point.x <= x + width &&
			   point.y >= y && point.y <= y + height;
	}

	bool intersects( const Rectangle& box ) const
	{
		return x < box.x + box.width && x + width > box.x &&
			   y < box.y + box.height && y + height > box.y;
	}

	bool intersects( const Circle& circle ) const
	{
		float closestX = std::max( x, std::min( circle.x, x + width ) );
		float closestY = std::max( y, std::min( circle.y, y + height ) );

		float dx = closestX - circle.x;
		float dy = closestY - circle.y;

		float distanceSqr = dx * dx + dy * dy;

		return distanceSqr <= circle.radius * circle.radius;
	}

public:

	float x = 0.0f;
	float y = 0.0f;
	float width = 1.0f;
	float height = 1.0f;
};

class QuadTree
{
public:

	QuadTree( const Rectangle& boundary )
	{
		m_boundary = boundary;

		rectShape.setPosition( m_boundary.x, m_boundary.y );
		rectShape.setSize( sf::Vector2f( m_boundary.width, m_boundary.height ) );
		rectShape.setFillColor( sf::Color::Transparent );
		rectShape.setOutlineColor( sf::Color( 217, 217, 217 ) );
		rectShape.setOutlineThickness( 1.0f );

		pointShape.setRadius( 3.0f );
		pointShape.setFillColor( sf::Color( 217, 217, 217 ) );
		pointShape.setOrigin( pointShape.getRadius(), pointShape.getRadius() );
	}

	~QuadTree()
	{
		delete m_top_left;
		delete m_top_right;
		delete m_bottom_left;
		delete m_bottom_right;
	}

public:

	bool insert( const Point& point )
	{
		if ( !m_boundary.contains( point ) ) return false;

		if ( m_points.size() < m_capacity )
		{
			m_points.push_back( point );
			return true;
		}
		else
		{
			subdivide();

			if ( m_top_left->insert( point ) ) return true;
			if ( m_top_right->insert( point ) ) return true;
			if ( m_bottom_left->insert( point ) ) return true;
			if ( m_bottom_right->insert( point ) ) return true;

			return false;
		}
	}

	void query( const Rectangle& range, std::vector<Point>& points ) const
	{
		if ( !m_boundary.intersects( range ) ) return;

		for ( const auto& point : m_points )
		{
			checkCount++;
			if ( range.contains( point ) ) points.push_back( point );
		}

		if ( m_top_left )
		{
			m_top_left->query( range, points );
			m_top_right->query( range, points );
			m_bottom_left->query( range, points );
			m_bottom_right->query( range, points );
		}
	}

	void query( const Circle& range, std::vector<Point>& points ) const
	{
		if ( !m_boundary.intersects( range ) ) return;

		for ( const auto& point : m_points )
		{
			checkCount++;
			if ( range.contains( point ) ) points.push_back( point );
		}

		if ( m_top_left )
		{
			m_top_left->query( range, points );
			m_top_right->query( range, points );
			m_bottom_left->query( range, points );
			m_bottom_right->query( range, points );
		}
	}

	void clear()
	{
		m_points.clear();

		if ( m_top_left )
		{
			delete m_top_left;
			m_top_left = nullptr;

			delete m_top_right;
			m_top_right = nullptr;

			delete m_bottom_left;
			m_bottom_left = nullptr;

			delete m_bottom_right;
			m_bottom_right = nullptr;
		}
	}

	void drawQuads( sf::RenderWindow& window ) const
	{
		window.draw( rectShape );

		if ( !m_top_left ) return;
		
		m_top_left->drawQuads( window );
		m_top_right->drawQuads( window );
		m_bottom_left->drawQuads( window );
		m_bottom_right->drawQuads( window );
	}

	void drawPoints( sf::RenderWindow& window )
	{
		for ( const auto& point : m_points )
		{
			pointShape.setPosition( point.x, point.y );
			window.draw( pointShape );
		}

		if ( !m_top_left ) return;
		
		m_top_left->drawPoints( window );
		m_top_right->drawPoints( window );
		m_bottom_left->drawPoints( window );
		m_bottom_right->drawPoints( window );
	}

private:

	void subdivide()
	{
		// if already divided - return
		if ( m_top_left != nullptr ) return;

		float x = m_boundary.x;
		float y = m_boundary.y;
		float w = m_boundary.width * 0.5f;
		float h = m_boundary.height * 0.5f;

		m_top_left = new QuadTree( Rectangle( x, y, w, h ) );
		m_top_right = new QuadTree( Rectangle( x + w, y, w, h ) );
		m_bottom_left = new QuadTree( Rectangle( x, y + h, w, h ) );
		m_bottom_right = new QuadTree( Rectangle( x + w, y + h, w, h ) );
	}

private:

	sf::RectangleShape rectShape;
	sf::CircleShape pointShape;

private:

	Rectangle m_boundary;

	int m_capacity = 4;
	std::vector<Point> m_points;

	QuadTree* m_top_left = nullptr;
	QuadTree* m_top_right = nullptr;
	QuadTree* m_bottom_left = nullptr;
	QuadTree* m_bottom_right = nullptr;
};

void spawnRandomPoints( QuadTree& quadtree )
{
	for ( int i = 0; i < spawnCount; ++i )
	{
		float x = random( 0.0f, win_width );
		float y = random( 0.0f, win_height );

		quadtree.insert( Point( x, y ) );
		pointCount++;
	}
}

int main()
{
	sf::RenderWindow window( sf::VideoMode( win_width, win_height ), "quadtree" );

	std::srand( std::time( nullptr ) );

	sf::Clock clock;

	sf::Font font;
	if ( !font.loadFromFile( "inter-regular.otf" ) ) return -1;

	sf::Text text( "", font, 16 );
	text.setPosition( 10.0f, 12.0f );
	text.setFillColor( sf::Color::White );

	Rectangle boundary = Rectangle( 0.0f, 0.0f, win_width, win_height );
	QuadTree quadtree = QuadTree( boundary );

	spawnRandomPoints( quadtree );

	sf::RectangleShape selectBox;
	selectBox.setFillColor( sf::Color( 0, 120, 215, 50 ) );
	selectBox.setOutlineColor( sf::Color( 0, 120, 215 ) );
	selectBox.setOutlineThickness( 1.0f );

	sf::CircleShape selectCircle;
	selectCircle.setFillColor( sf::Color( 0, 120, 215, 50 ) );
	selectCircle.setOutlineColor( sf::Color( 0, 120, 215 ) );
	selectCircle.setOutlineThickness( 1.0f );

	sf::CircleShape pointShape( 3.5f );
	pointShape.setFillColor( sf::Color( 0, 255, 127 ) );
	pointShape.setOrigin( pointShape.getRadius(), pointShape.getRadius() );

	bool prevState = false;
	bool drawSelect = false;
	float radius = 100.0f;

	sf::Vector2i selectStartPosition;

	while ( window.isOpen() )
    {
		float deltaTime = clock.restart().asSeconds();

		char titleText[255];
		sprintf( titleText, "quadtree (%.2f fps / %.2f ms)", 1.0f / deltaTime, deltaTime * 1000 );

		window.setTitle( titleText );

        sf::Event event;
        
		while ( window.pollEvent( event ) )
        {
            if ( event.type == sf::Event::Closed ) window.close();

			if ( event.type == sf::Event::KeyPressed )
			{
				if ( event.key.code == sf::Keyboard::F1 )
				{
					selectMode = SelectMode::MouseSelectBox;
				}
				else if ( event.key.code == sf::Keyboard::F2 )
				{
					selectMode = SelectMode::MouseAroundBox;
				}
				else if ( event.key.code == sf::Keyboard::F3 )
				{
					selectMode = SelectMode::MouseAroundCircle;
				}
				else if ( event.key.code == sf::Keyboard::F5 )
				{
					quadtree.clear();
					pointCount = 0;
				}
				else if ( event.key.code ==  sf::Keyboard::F6 )
				{
					drawQuadTreeQuads = !drawQuadTreeQuads;
				}
				else if ( event.key.code ==  sf::Keyboard::F7 )
				{
					drawQuadTreePoints = !drawQuadTreePoints;
				}
				else if ( event.key.code == sf::Keyboard::F8 )
				{
					drawSelectedPoints = !drawSelectedPoints;
				}
			}

			if ( event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel )
			{
				radius += event.mouseWheelScroll.delta * 10.0f;
				radius = std::max( 50.0f, std::min( radius, 200.0f ) );
			}
        }

		if ( sf::Keyboard::isKeyPressed( sf::Keyboard::F4 ) )
		{
			spawnRandomPoints( quadtree );
		}
	
		if ( sf::Mouse::isButtonPressed( sf::Mouse::Left ) )
		{
			// event pressed
			if ( prevState == false )
			{
				prevState = true;
				drawSelect = true;

				if ( selectMode == SelectMode::MouseSelectBox )
				{
					selectStartPosition = sf::Mouse::getPosition( window );
					selectBox.setPosition( selectStartPosition.x, selectStartPosition.y );
				}
			}
		}
		else
		{
			// event released
			if ( prevState == true )
			{
				prevState = false;
				drawSelect = false;
			}
		}

		if ( sf::Mouse::isButtonPressed( sf::Mouse::Right ) )
		{
			sf::Vector2i cursorPosition = sf::Mouse::getPosition( window );

			float x = cursorPosition.x + random( -10, 10 );
			float y = cursorPosition.y + random( -10, 10 );

			quadtree.insert( Point( x, y ) );

			pointCount++;
        }

		checkCount = 0;
		selectedCount = 0;

        window.clear( sf::Color( 30, 30, 30 ) );

		if ( drawQuadTreeQuads ) quadtree.drawQuads( window );
		if ( drawQuadTreePoints ) quadtree.drawPoints( window );

		// range initializtion

		sf::Vector2i cursorPosition = sf::Mouse::getPosition( window );

		Rectangle boxRange = {};
		Circle circleRange = {};

		if ( selectMode == SelectMode::MouseSelectBox )
		{
			boxRange.x = std::min( selectStartPosition.x, cursorPosition.x );
			boxRange.y = std::min( selectStartPosition.y, cursorPosition.y );
			boxRange.width = std::abs( cursorPosition.x - selectStartPosition.x );
			boxRange.height = std::abs( cursorPosition.y - selectStartPosition.y );
		}
		else if ( selectMode == SelectMode::MouseAroundBox )
		{
			boxRange.x = cursorPosition.x - radius;
			boxRange.y = cursorPosition.y - radius;
			boxRange.width = radius * 2.0f;
			boxRange.height = radius * 2.0f;
		}
		else if ( selectMode == SelectMode::MouseAroundCircle )
		{
			circleRange.x = cursorPosition.x;
			circleRange.y = cursorPosition.y;
			circleRange.radius = radius;
		}

		// draw range

		if ( selectMode == SelectMode::MouseSelectBox )
		{
			if ( drawSelect )
			{
				selectBox.setPosition( boxRange.x, boxRange.y );
				selectBox.setSize( sf::Vector2f( boxRange.width, boxRange.height ) );
				window.draw( selectBox );
			}
		}
		else if ( selectMode == SelectMode::MouseAroundBox )
		{
			selectBox.setPosition( boxRange.x, boxRange.y );
			selectBox.setSize( sf::Vector2f( boxRange.width, boxRange.height ) );
			window.draw( selectBox );
		}
		else if ( selectMode == SelectMode::MouseAroundCircle )
		{
			selectCircle.setPosition( circleRange.x, circleRange.y );
			selectCircle.setRadius( circleRange.radius );
			selectCircle.setOrigin( circleRange.radius, circleRange.radius );
			window.draw( selectCircle );
		}

		// query

		std::vector<Point> points;

		if ( selectMode == SelectMode::MouseSelectBox )
		{
			if ( drawSelect )
			{
				quadtree.query( boxRange, points );
			}
		}
		else if ( selectMode == SelectMode::MouseAroundBox )
		{
			quadtree.query( boxRange, points );
		}
		else if ( selectMode == SelectMode::MouseAroundCircle )
		{
			quadtree.query( circleRange, points );
		}

		selectedCount = points.size();

		// draw selected points

		if ( drawSelectedPoints )
		{
			if ( selectMode == SelectMode::MouseSelectBox )
			{
				if ( drawSelect )
				{
					for ( const auto& point : points )
					{
						pointShape.setPosition( point.x, point.y );
						window.draw( pointShape );
					}
				}
			}
			else
			{
				for ( const auto& point : points )
				{
					pointShape.setPosition( point.x, point.y );
					window.draw( pointShape );
				}
			}
		}

		// draw debug text

		std::string selectModeText;

		switch ( selectMode )
		{
		case SelectMode::MouseSelectBox:
			selectModeText = "SelectBox";
			break;
		case SelectMode::MouseAroundBox:
			selectModeText = "BoxAroundCursor";
			break;
		case SelectMode::MouseAroundCircle:
			selectModeText = "CircleAroundCursor";
			break;
		}

		std::string textString =
			"selectMode: " + selectModeText +
			"\npointCount: " + std::to_string( pointCount ) +
			"\ncheckCount: " + std::to_string( checkCount ) +
			"\nselectedCount: " + std::to_string( selectedCount );

		text.setString( textString );
		window.draw( text );

		window.display();
    }

	return 0;
}

