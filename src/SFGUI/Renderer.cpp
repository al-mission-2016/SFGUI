#include <GL/glew.h>
#include <SFGUI/Renderer.hpp>
#include <SFGUI/Context.hpp>
#include <SFGUI/RendererViewport.hpp>
#include <SFGUI/Engine.hpp>

#include <cmath>
#include <cstring>

namespace sfg {

Renderer::Renderer() :
	m_last_vertex_count( 0 ),
	m_alpha_threshold( 0.f ),
	m_depth_clear_strategy( NO_DEPTH ),
	m_vbo_synced( false ),
	m_preblend( false ),
	m_cull( false ) {
	glGenBuffers( 1, &m_vertex_vbo );
	glGenBuffers( 1, &m_color_vbo );
	glGenBuffers( 1, &m_texture_vbo );

	m_default_viewport = CreateViewport();

	// Load our "no texture" pseudo-texture.
	sf::Image image;
	image.Create( 2, 2, sf::Color::White );
	LoadImage( image, sf::Color::White );
}

Renderer::~Renderer() {
	glDeleteBuffers( 1, &m_texture_vbo );
	glDeleteBuffers( 1, &m_color_vbo );
	glDeleteBuffers( 1, &m_vertex_vbo );
}

const RendererViewport::Ptr& Renderer::GetDefaultViewport() {
	return m_default_viewport;
}

RendererViewport::Ptr Renderer::CreateViewport() {
	RendererViewport::Ptr viewport( new RendererViewport );

	return viewport;
}

Primitive::Ptr Renderer::CreateText( const sf::Text& text, sf::Color background_color_hint ) {
	Primitive::Ptr primitive( new Primitive );

	const sf::Font& font = text.GetFont();
	unsigned int character_size = text.GetCharacterSize();
	sf::Color color = text.GetColor();

	if( m_preblend ) {
		color = sf::Color::White;
	}

	sf::Vector2f atlas_offset = LoadFont( font, character_size, background_color_hint, text.GetColor() );

	const sf::String& str = text.GetString();
	std::size_t length = str.GetSize();

	float horizontal_spacing = static_cast<float>( font.GetGlyph( L' ', character_size, false ).Advance );
	float vertical_spacing = static_cast<float>( font.GetLineSpacing( character_size ) );
	sf::Vector2f position( std::floor( text.GetPosition().x + .5f ), std::floor( text.GetPosition().y + static_cast<float>( character_size ) + .5f ) );

	const static float tab_spaces = 2.f;

	sf::Uint32 previous_character = 0;

	for( std::size_t index = 0; index < length; ++index ) {
		sf::Uint32 current_character = str[index];

		position.x += static_cast<float>( font.GetKerning( previous_character, current_character, character_size ) );

		switch( current_character ) {
			case L' ':
				position.x += horizontal_spacing;
				continue;
			case L'\t':
				position.x += horizontal_spacing * tab_spaces;
				continue;
			case L'\n':
				position.y += vertical_spacing;
				position.x = 0.f;
				continue;
			case L'\v':
				position.y += vertical_spacing * tab_spaces;
				continue;
			default:
				break;
		}

		const sf::Glyph& glyph = font.GetGlyph( current_character, character_size, false );

		Primitive::Vertex vertex0;
		Primitive::Vertex vertex1;
		Primitive::Vertex vertex2;
		Primitive::Vertex vertex3;

		vertex0.position = position + sf::Vector2f( static_cast<float>( glyph.Bounds.Left ), static_cast<float>( glyph.Bounds.Top ) );
		vertex1.position = position + sf::Vector2f( static_cast<float>( glyph.Bounds.Left ), static_cast<float>( glyph.Bounds.Top + glyph.Bounds.Height ) );
		vertex2.position = position + sf::Vector2f( static_cast<float>( glyph.Bounds.Left + glyph.Bounds.Width ), static_cast<float>( glyph.Bounds.Top ) );
		vertex3.position = position + sf::Vector2f( static_cast<float>( glyph.Bounds.Left + glyph.Bounds.Width ), static_cast<float>( glyph.Bounds.Top + glyph.Bounds.Height ) );

		vertex0.color = color;
		vertex1.color = color;
		vertex2.color = color;
		vertex3.color = color;

		// Let SFML cast the Rect for us.
		sf::FloatRect texture_rect( glyph.TextureRect );

		vertex0.texture_coordinate = atlas_offset + sf::Vector2f( texture_rect.Left, texture_rect.Top );
		vertex1.texture_coordinate = atlas_offset + sf::Vector2f( texture_rect.Left, texture_rect.Top + texture_rect.Height );
		vertex2.texture_coordinate = atlas_offset + sf::Vector2f( texture_rect.Left + texture_rect.Width, texture_rect.Top );
		vertex3.texture_coordinate = atlas_offset + sf::Vector2f( texture_rect.Left + texture_rect.Width, texture_rect.Top + texture_rect.Height );

		primitive->AddVertex( vertex0 );
		primitive->AddVertex( vertex1 );
		primitive->AddVertex( vertex2 );
		primitive->AddVertex( vertex2 );
		primitive->AddVertex( vertex1 );
		primitive->AddVertex( vertex3 );

		position.x += static_cast<float>( glyph.Advance );

		previous_character = current_character;
	}

	m_primitives.push_back( primitive );

	return primitive;
}

Primitive::Ptr Renderer::CreateQuad( const sf::Vector2f& top_left, const sf::Vector2f& bottom_left,
                                             const sf::Vector2f& bottom_right, const sf::Vector2f& top_right,
                                             const sf::Color& color ) {
	Primitive::Ptr primitive( new Primitive );

	Primitive::Vertex vertex0;
	Primitive::Vertex vertex1;
	Primitive::Vertex vertex2;
	Primitive::Vertex vertex3;

	vertex0.position = sf::Vector2f( std::floor( top_left.x + .5f ), std::floor( top_left.y + .5f ) );
	vertex1.position = sf::Vector2f( std::floor( bottom_left.x + .5f ), std::floor( bottom_left.y + .5f ) );
	vertex2.position = sf::Vector2f( std::floor( top_right.x + .5f ), std::floor( top_right.y + .5f ) );
	vertex3.position = sf::Vector2f( std::floor( bottom_right.x + .5f ), std::floor( bottom_right.y + .5f ) );

	vertex0.color = color;
	vertex1.color = color;
	vertex2.color = color;
	vertex3.color = color;

	vertex0.texture_coordinate = sf::Vector2f( 0.f, 0.f );
	vertex1.texture_coordinate = sf::Vector2f( 0.f, 1.f );
	vertex2.texture_coordinate = sf::Vector2f( 1.f, 0.f );
	vertex3.texture_coordinate = sf::Vector2f( 1.f, 1.f );

	primitive->AddVertex( vertex0 );
	primitive->AddVertex( vertex1 );
	primitive->AddVertex( vertex2 );
	primitive->AddVertex( vertex2 );
	primitive->AddVertex( vertex1 );
	primitive->AddVertex( vertex3 );

	m_primitives.push_back( primitive );

	return primitive;
}

Primitive::Ptr Renderer::CreatePane( const sf::Vector2f& position, const sf::Vector2f& size, float border_width,
                                             const sf::Color& color, const sf::Color& border_color, int border_color_shift ) {
  if( border_width <= 0.f ) {
		return CreateRect( position, position + size, color );
  }

  Primitive::Ptr primitive( new Primitive );

	sf::Color dark_border( border_color );
	sf::Color light_border( border_color );

	Context::Get().GetEngine().ShiftBorderColors( light_border, dark_border, border_color_shift );

	float left = position.x;
	float top = position.y;
	float right = left + size.x;
	float bottom = top + size.y;

	Primitive::Ptr rect(
		CreateQuad(
			sf::Vector2f( left + border_width, top + border_width ),
			sf::Vector2f( left + border_width, bottom - border_width ),
			sf::Vector2f( right - border_width, bottom - border_width ),
			sf::Vector2f( right - border_width, top + border_width ),
			color
		)
	);

	Primitive::Ptr line_top(
		CreateLine(
			sf::Vector2f( left, top + border_width / 2.f ),
			sf::Vector2f( right - border_width, top + border_width / 2.f ),
			light_border,
			border_width
		)
	);

	Primitive::Ptr line_right(
		CreateLine(
			sf::Vector2f( right - border_width / 2.f, top ),
			sf::Vector2f( right - border_width / 2.f, bottom - border_width ),
			dark_border,
			border_width
		)
	);

	Primitive::Ptr line_bottom(
		CreateLine(
			sf::Vector2f( right, bottom - border_width / 2.f ),
			sf::Vector2f( left + border_width, bottom - border_width / 2.f ),
			dark_border,
			border_width
		)
	);

	Primitive::Ptr line_left(
		CreateLine(
			sf::Vector2f( left + border_width / 2.f, bottom ),
			sf::Vector2f( left + border_width / 2.f, top + border_width ),
			light_border,
			border_width
		)
	);

	primitive->Add( *rect.get() );
	primitive->Add( *line_top.get() );
	primitive->Add( *line_right.get() );
	primitive->Add( *line_bottom.get() );
	primitive->Add( *line_left.get() );

	std::vector<Primitive::Ptr>::iterator iter( std::find( m_primitives.begin(), m_primitives.end(), rect ) );

	assert( iter != m_primitives.end() );

	iter = m_primitives.erase( iter ); // rect
	iter = m_primitives.erase( iter ); // line_top
	iter = m_primitives.erase( iter ); // line_right
	iter = m_primitives.erase( iter ); // line_bottom
	m_primitives.erase( iter ); // line_left

	m_primitives.push_back( primitive );

	return primitive;
}

Primitive::Ptr Renderer::CreateRect( const sf::Vector2f& top_left, const sf::Vector2f& bottom_right, const sf::Color& color ) {
	return CreateQuad(
		sf::Vector2f( top_left.x, top_left.y ),
		sf::Vector2f( top_left.x, bottom_right.y ),
		sf::Vector2f( bottom_right.x, bottom_right.y ),
		sf::Vector2f( bottom_right.x, top_left.y ),
		color
	);
}

Primitive::Ptr Renderer::CreateRect( const sf::FloatRect& rect, const sf::Color& color ) {
	return CreateRect( sf::Vector2f( rect.Left, rect.Top ), sf::Vector2f( rect.Left + rect.Width, rect.Top + rect.Height ), color );
}

Primitive::Ptr Renderer::CreateTriangle( const sf::Vector2f& point0, const sf::Vector2f& point1, const sf::Vector2f& point2, const sf::Color& color ) {
	Primitive::Ptr primitive( new Primitive );

	Primitive::Vertex vertex0;
	Primitive::Vertex vertex1;
	Primitive::Vertex vertex2;

	vertex0.position = point0;
	vertex1.position = point1;
	vertex2.position = point2;

	vertex0.color = color;
	vertex1.color = color;
	vertex2.color = color;

	vertex0.texture_coordinate = sf::Vector2f( 0.f, 0.f );
	vertex1.texture_coordinate = sf::Vector2f( 0.f, 1.f );
	vertex2.texture_coordinate = sf::Vector2f( 1.f, 0.f );

	primitive->AddVertex( vertex0 );
	primitive->AddVertex( vertex1 );
	primitive->AddVertex( vertex2 );

	m_primitives.push_back( primitive );

	return primitive;
}

Primitive::Ptr Renderer::CreateImage( const sf::FloatRect& rect, const sf::Image& image, sf::Color background_color_hint ) {
	sf::Vector2f offset( LoadImage( image, background_color_hint ) );

	Primitive::Ptr primitive( new Primitive );

	Primitive::Vertex vertex0;
	Primitive::Vertex vertex1;
	Primitive::Vertex vertex2;
	Primitive::Vertex vertex3;

	vertex0.position = sf::Vector2f( std::floor( rect.Left + .5f ), std::floor( rect.Top + .5f ) );
	vertex1.position = sf::Vector2f( std::floor( rect.Left + .5f ), std::floor( rect.Top + .5f ) + std::floor( rect.Height + .5f ) );
	vertex2.position = sf::Vector2f( std::floor( rect.Left + .5f ) + std::floor( rect.Width + .5f ), std::floor( rect.Top + .5f ) );
	vertex3.position = sf::Vector2f( std::floor( rect.Left + .5f ) + std::floor( rect.Width + .5f ), std::floor( rect.Top + .5f ) + std::floor( rect.Height + .5f ) );

	vertex0.color = sf::Color( 255, 255, 255, 255 );
	vertex1.color = sf::Color( 255, 255, 255, 255 );
	vertex2.color = sf::Color( 255, 255, 255, 255 );
	vertex3.color = sf::Color( 255, 255, 255, 255 );

	vertex0.texture_coordinate = offset + sf::Vector2f( 0.f, 0.f );
	vertex1.texture_coordinate = offset + sf::Vector2f( 0.f, static_cast<float>( image.GetHeight() ) );
	vertex2.texture_coordinate = offset + sf::Vector2f( static_cast<float>( image.GetWidth() ), 0.f );
	vertex3.texture_coordinate = offset + sf::Vector2f( static_cast<float>( image.GetWidth() ), static_cast<float>( image.GetHeight() ) );

	primitive->AddVertex( vertex0 );
	primitive->AddVertex( vertex1 );
	primitive->AddVertex( vertex2 );
	primitive->AddVertex( vertex2 );
	primitive->AddVertex( vertex1 );
	primitive->AddVertex( vertex3 );

	m_primitives.push_back( primitive );

	return primitive;
}

Primitive::Ptr Renderer::CreateLine( const sf::Vector2f& begin, const sf::Vector2f& end, const sf::Color& color, float thickness ) {
	// Get vector perpendicular to direction of the line vector.
	// Vector is rotated CCW 90 degrees and normalized.
	sf::Vector2f normal( end - begin );
	std::swap( normal.x, normal.y );
	float length = std::sqrt( normal.x * normal.x + normal.y * normal.y );
	normal.x /= -length;
	normal.y /= length;

	sf::Vector2f corner0( begin + normal * ( thickness * .5f ) );
	sf::Vector2f corner1( begin - normal * ( thickness * .5f ) );
	sf::Vector2f corner2( end - normal * ( thickness * .5f ) );
	sf::Vector2f corner3( end + normal * ( thickness * .5f ) );

	return CreateQuad( corner3, corner2, corner1, corner0, color );
}

void Renderer::Display( sf::RenderWindow& window ) {
	// Refresh VBO data if out of sync
	if( !m_vbo_synced ) {
		RefreshVBO( window );

		m_vbo_synced = true;
	}

	// Thanks to color / texture modulation we can draw the entire
	// frame in a single pass by pseudo-disabling the texturing with
	// the help of a white texture ( 1.f * something = something ).
	// Further, we stick all referenced textures into our giant atlas
	// so we don't have to rebind during the draw.

	SetupGL( window );

	m_texture_atlas.Bind();

	glBindBuffer( GL_ARRAY_BUFFER, m_vertex_vbo );
	glVertexPointer( 3, GL_FLOAT, 0, 0 );

	glBindBuffer( GL_ARRAY_BUFFER, m_color_vbo );
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, 0 );

	glBindBuffer( GL_ARRAY_BUFFER, m_texture_vbo );
	glTexCoordPointer( 2, GL_FLOAT, 0, 0 );

	// Not needed, constantly kept enabled by SFML... -_-
	//glEnableClientState( GL_VERTEX_ARRAY );
	//glEnableClientState( GL_COLOR_ARRAY );
	//glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	std::size_t scissor_pairs_size = m_viewport_pairs.size();

	glEnable( GL_SCISSOR_TEST );

	for( std::size_t index = 0; index < scissor_pairs_size; ++index ) {
		const ViewportPair& scissor_pair = m_viewport_pairs[index];

		RendererViewport::Ptr viewport = scissor_pair.first;

		if( viewport && ( viewport != m_default_viewport ) ) {
			sf::Vector2f destination_origin = viewport->GetDestinationOrigin();
			sf::Vector2f size = viewport->GetSize();

			glScissor(
				static_cast<int>( destination_origin.x ),
				window.GetHeight() - static_cast<int>( destination_origin.y + size.y ),
				static_cast<int>( size.x ),
				static_cast<int>( size.y )
			);
		}
		else {
			glScissor( 0, 0, window.GetWidth(), window.GetHeight() );
		}

		if( index < scissor_pairs_size - 1 ) {
			glDrawArrays( GL_TRIANGLES, scissor_pair.second, m_viewport_pairs[index + 1].second - scissor_pair.second );
		}
		else {
			glDrawArrays( GL_TRIANGLES, scissor_pair.second, m_last_vertex_count - scissor_pair.second );
		}
	}

	glDisable( GL_SCISSOR_TEST );

	//glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	//glDisableClientState( GL_COLOR_ARRAY );
	//glDisableClientState( GL_VERTEX_ARRAY );

	// Needed otherwise SFML will blow up...
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	RestoreGL( window );
}

void Renderer::SetupGL( sf::RenderWindow& window ) {
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	// When SFML dies (closes) it sets these to 0 for some reason.
	// That then causes glOrtho errors.
	unsigned int width = window.GetWidth();
	unsigned int height = window.GetHeight();

	glOrtho( 0.0f, static_cast<GLdouble>( width ? width : 1 ), static_cast<GLdouble>( height ? height : 1 ), 0.0f, -1.0f, 64.0f );

	glMatrixMode( GL_TEXTURE );
	glPushMatrix();
	glLoadIdentity();

	if( m_preblend ) {
		glDisable( GL_BLEND );
	}

	if( m_depth_clear_strategy ) {
		glEnable( GL_DEPTH_TEST );

		if( m_depth_clear_strategy & CLEAR_DEPTH ) {
			glClear( GL_DEPTH_BUFFER_BIT );
		}
		else if( m_depth_clear_strategy & ALTERNATE_DEPTH ) {
			if( m_depth_alternate_flag ) {
				glDepthFunc( GL_LESS );
				glDepthRange( 0.0, 0.5 );
			}
			else {
				glDepthFunc( GL_GREATER );
				glDepthRange( 1.0, 0.5 );
			}

			m_depth_alternate_flag = !m_depth_alternate_flag;
		}
	}

	if( m_alpha_threshold > 0.f ) {
		glAlphaFunc( GL_GREATER, m_alpha_threshold );
	}

	glEnable( GL_CULL_FACE );
}

void Renderer::RestoreGL( sf::RenderWindow& window ) {
	glDisable( GL_CULL_FACE );

	if( m_alpha_threshold > 0.f ) {
		glAlphaFunc( GL_GREATER, 0.f );
	}

	if( m_depth_clear_strategy ) {
		glDisable( GL_DEPTH_TEST );
	}

	if( m_preblend ) {
		glEnable( GL_BLEND );
	}

	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );

	// Make SFML disable it's **************** vertex cache without us
	// having to call ResetGLStates() and disturbing OpenGL needlessly.
	// This would be sooo much easier if we could somehow set
	// myCache.UseVertexCache = false;
	// in window by ourself every frame.
	// SFML doesn't like to play nice with other VBO / Vertex Array
	// users, that's for sure... It assumes that the array pointers
	// don't get changed between calls to Draw() unless ResetGLStates()
	// is explicitly called in between. Why do we need to call OpenGL
	// 10+ times just to reset something this simple? No logic.

	//static sf::VertexArray resetter_array( sf::TrianglesStrip, 5 );
	//window.Draw( resetter_array );

	// Or... even more evil... use memset to blank the StatesCache of
	// the RenderWindow with zeros. Thankfully, because we are using
	// the data structures directly from the SFML headers, if Laurent
	// decides to change their size one day we won't even notice.
	struct StatesCache {
		bool ViewChanged;
		sf::BlendMode LastBlendMode;
		sf::Uint64 LastTextureId;
		bool UseVertexCache;
		sf::Vertex VertexCache[4];
	};

	// All your cache are belong to us.
	memset( reinterpret_cast<char*>( &window ) + sizeof( sf::RenderTarget ) - sizeof( StatesCache ), 0, sizeof( StatesCache ) );
}

sf::Vector2f Renderer::LoadFont( const sf::Font& font, unsigned int size, sf::Color background_color_hint, sf::Color foreground_color_hint ) {
	// Make sure all the glyphs we need are loaded.
	for( sf::Uint32 codepoint = 0; codepoint < 512; ++codepoint ) {
		font.GetGlyph( codepoint, size, false );
	}

	sf::Image image = font.GetTexture( size ).CopyToImage();

	return LoadImage( image, background_color_hint, foreground_color_hint );
}

sf::Vector2f Renderer::LoadImage( const sf::Image& image, sf::Color background_color_hint, sf::Color foreground_color_hint ) {
	sf::Image preblended_image;

	preblended_image.Create( image.GetWidth(), image.GetHeight(), image.GetPixelsPtr() );

	// If we get a proper background color hint and preblend is enabled,
	// precompute blended color with ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ).
	if( m_preblend ) {
		if( background_color_hint.a == 255 ) {
			float foreground_r_factor = 1.f;
			float foreground_g_factor = 1.f;
			float foreground_b_factor = 1.f;

			if( foreground_color_hint.a == 255 ) {
				foreground_r_factor = static_cast<float>( foreground_color_hint.r ) / 255.f;
				foreground_g_factor = static_cast<float>( foreground_color_hint.g ) / 255.f;
				foreground_b_factor = static_cast<float>( foreground_color_hint.b ) / 255.f;
			}

			std::size_t pixel_count = preblended_image.GetWidth() * preblended_image.GetHeight();

			sf::Uint8* bytes = new sf::Uint8[pixel_count * 4];

			memcpy( bytes, preblended_image.GetPixelsPtr(), pixel_count * 4 );

			for( std::size_t index = 0; index < pixel_count; ++index ) {
				// Alpha
				float alpha = static_cast<float>( bytes[index * 4 + 3] ) / 255.f;

				// Red
				bytes[index * 4 + 0] = static_cast<sf::Uint8>( static_cast<float>( bytes[index * 4 + 0] ) * foreground_r_factor * alpha + static_cast<float>( background_color_hint.r ) * ( 1.f - alpha ) );

				// Green
				bytes[index * 4 + 1] = static_cast<sf::Uint8>( static_cast<float>( bytes[index * 4 + 1] ) * foreground_g_factor * alpha + static_cast<float>( background_color_hint.g ) * ( 1.f - alpha ) );

				// Blue
				bytes[index * 4 + 2] = static_cast<sf::Uint8>( static_cast<float>( bytes[index * 4 + 2] ) * foreground_b_factor * alpha + static_cast<float>( background_color_hint.b ) * ( 1.f - alpha ) );
			}

			preblended_image.Create( preblended_image.GetWidth(), preblended_image.GetHeight(), bytes );

			delete[] bytes;
		}
		else {
			m_preblend = false;

#ifdef SFGUI_DEBUG
			std::cerr << "Detected alpha value " << static_cast<int>( background_color_hint.a ) << " in background color hint. Disabling preblend.\n";
#endif
		}
	}

	const sf::Uint8* bytes = preblended_image.GetPixelsPtr();
	std::size_t byte_count = preblended_image.GetWidth() * preblended_image.GetHeight() * 4;

	unsigned long hash = 2166136261UL;

	// Disable this check for now.
	static sf::Uint8 alpha_threshold = 255;

	for ( ; byte_count; --byte_count ) {
		hash ^= static_cast<unsigned long>( bytes[ byte_count - 1 ] );
		hash *= 16777619UL;

		// Check if the image makes intentional use of the alpha channel.
		if( m_depth_clear_strategy && !( byte_count % 4 ) && ( bytes[ byte_count - 1 ] > alpha_threshold ) && ( bytes[ byte_count - 1 ] < 255 ) ) {
#ifdef SFGUI_DEBUG
			std::cerr << "Detected alpha value " << static_cast<int>( bytes[ byte_count - 1 ]  ) << " in texture, disabling depth test.\n";
#endif
			m_depth_clear_strategy = false;
		}
	}

	if( m_atlas_offsets.find( hash ) == m_atlas_offsets.end() ) {
		// Image needs to be loaded into atlas.
		sf::Image old_image = m_texture_atlas.CopyToImage();
		sf::Image new_image;

		// We insert padding between atlas elements to prevent
		// texture filtering from screwing up our images.
		// If 1 pixel isn't enough, increase.
		const static unsigned int padding = 1;

		new_image.Create( std::max( old_image.GetWidth(), preblended_image.GetWidth() ), old_image.GetHeight() + preblended_image.GetHeight() + padding, sf::Color::White );
		new_image.Copy( old_image, 0, 0 );

		new_image.Copy( preblended_image, 0, old_image.GetHeight() + padding );

		m_texture_atlas.LoadFromImage( new_image );

		m_atlas_offsets[hash] = sf::Vector2f( 0.f, static_cast<float>( old_image.GetHeight() + padding ) );

		InvalidateVBO();
	}

	return m_atlas_offsets[hash];
}

void Renderer::SortPrimitives() {
	std::size_t current_position = 1;
	std::size_t sort_index;

	std::size_t primitives_size = m_primitives.size();

	// Classic insertion sort.
	while( current_position < primitives_size ) {
		sort_index = current_position++;

		while( ( sort_index > 0 ) && ( m_primitives[sort_index - 1]->GetLevel() > m_primitives[sort_index]->GetLevel() ) ) {
			m_primitives[sort_index].swap( m_primitives[sort_index - 1] );
			--sort_index;
		}
	}
}

void Renderer::RefreshVBO( sf::RenderWindow& window ) {
	SortPrimitives();

	m_vertex_data.clear();
	m_color_data.clear();
	m_texture_data.clear();

	m_viewport_pairs.clear();

	m_last_vertex_count = 0;

	std::size_t primitives_size = m_primitives.size();

	// Check for alpha values in all primitives.
	// Disable depth test if any found.
	for( std::size_t primitive_index = 0; primitive_index < primitives_size; ++primitive_index ) {
		const std::vector<Primitive::Vertex>& vertices( m_primitives[primitive_index]->GetVertices() );

		std::size_t vertices_size = vertices.size();

		for( std::size_t vertex_index = 0; vertex_index < vertices_size; ++vertex_index ) {
			const Primitive::Vertex& vertex( vertices[vertex_index] );

			if( m_depth_clear_strategy && ( vertex.color.a < 255 ) ) {
#ifdef SFGUI_DEBUG
				std::cerr << "Detected alpha value " << static_cast<int>( vertex.color.a ) << " disabling depth test.\n";
#endif
				m_depth_clear_strategy = false;
			}
		}
	}

	// Used to normalize texture coordinates.
	sf::Vector2f normalizer( 1.f / static_cast<float>( m_texture_atlas.GetWidth() ), 1.f / static_cast<float>( m_texture_atlas.GetHeight() ) );

	// Depth test vars
	float depth = -4.f;
	float depth_delta = 4.f / static_cast<float>( primitives_size );
	int direction = m_depth_clear_strategy ? -1 : 1;
	int start = m_depth_clear_strategy ? primitives_size : 1;
	std::size_t end = m_depth_clear_strategy ? 0 : primitives_size + 1;

	RendererViewport::Ptr current_viewport = m_default_viewport;
	m_viewport_pairs.push_back( ViewportPair( m_default_viewport, 0 ) );

	sf::FloatRect window_viewport( 0.f, 0.f, static_cast<float>( window.GetWidth() ), static_cast<float>( window.GetHeight() ) );

	for( std::size_t primitive_index = start; primitive_index != end; primitive_index += direction ) {
		Primitive* primitive = m_primitives[primitive_index - 1].get();

		primitive->SetSynced();

		if( !primitive->IsVisible() ) {
			continue;
		}

		sf::Vector2f position_transform( primitive->GetPosition() );

		// Check if primitive needs to be rendered in a custom viewport.
		RendererViewport::Ptr viewport = primitive->GetViewport();

		if( viewport != current_viewport ) {
			current_viewport = viewport;

			ViewportPair scissor_pair( viewport, m_last_vertex_count );

			m_viewport_pairs.push_back( scissor_pair );
		}

		bool cull = m_cull;

		sf::FloatRect viewport_rect = window_viewport;

		if( viewport && ( viewport != m_default_viewport ) ) {
			sf::Vector2f destination_origin( viewport->GetDestinationOrigin() );
			sf::Vector2f size( viewport->GetSize() );

			position_transform += ( destination_origin - viewport->GetSourceOrigin() );

			if( m_cull ) {
				viewport_rect.Left = destination_origin.x;
				viewport_rect.Top = destination_origin.y;
				viewport_rect.Width = size.x;
				viewport_rect.Height = size.y;
			}
		}

		// Process primitive's vertices.
		const std::vector<Primitive::Vertex>& vertices( primitive->GetVertices() );

		std::size_t vertices_size = vertices.size();

		for( std::size_t vertex_index = 0; vertex_index < vertices_size; ++vertex_index ) {
			const Primitive::Vertex& vertex( vertices[vertex_index] );

			sf::Vector3f position( vertex.position.x + position_transform.x, vertex.position.y + position_transform.y, depth );

			m_vertex_data.push_back( position );
			m_color_data.push_back( vertex.color );

			// Normalize SFML's pixel texture coordinates.
			m_texture_data.push_back( sf::Vector2f( vertex.texture_coordinate.x * normalizer.x, vertex.texture_coordinate.y * normalizer.y ) );

			if( m_cull && viewport_rect.Contains( position.x, position.y ) ) {
				cull = false;
			}
		}

		if( cull ) {
			m_vertex_data.resize( m_last_vertex_count );
			m_color_data.resize( m_last_vertex_count );
			m_texture_data.resize( m_last_vertex_count );
		}
		else {
			m_last_vertex_count += vertices_size;
			depth -= depth_delta;
		}
	}

	// Sync vertex data
	glBindBuffer( GL_ARRAY_BUFFER, m_vertex_vbo );
	glBufferData( GL_ARRAY_BUFFER, m_vertex_data.size() * sizeof( sf::Vector3f ), 0, GL_DYNAMIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, 0, m_vertex_data.size() * sizeof( sf::Vector3f ), &m_vertex_data[0] );

	// Sync color data
	glBindBuffer( GL_ARRAY_BUFFER, m_color_vbo );
	glBufferData( GL_ARRAY_BUFFER, m_color_data.size() * sizeof( sf::Color ), 0, GL_DYNAMIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, 0, m_color_data.size() * sizeof( sf::Color ), &m_color_data[0] );

	// Sync texture coord data
	glBindBuffer( GL_ARRAY_BUFFER, m_texture_vbo );
	glBufferData( GL_ARRAY_BUFFER, m_texture_data.size() * sizeof( sf::Vector2f ), 0, GL_STATIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, 0, m_texture_data.size() * sizeof( sf::Vector2f ), &m_texture_data[0] );
}

void Renderer::RemovePrimitive( const Primitive::Ptr& primitive ) {
	std::vector<Primitive::Ptr>::iterator iter( std::find( m_primitives.begin(), m_primitives.end(), primitive ) );

	m_primitives.erase( iter );

	InvalidateVBO();
}

void Renderer::InvalidateVBO() {
	m_vbo_synced = false;
}

void Renderer::TuneDepthTest( unsigned char strategy ) {
	if( strategy & NO_DEPTH ) {
		m_depth_clear_strategy = strategy;
	}
	else if( strategy & CLEAR_DEPTH ) {
		m_depth_clear_strategy = strategy;
	}
	else if( strategy & ALTERNATE_DEPTH ) {
		m_depth_clear_strategy = strategy;

		m_depth_alternate_flag = true;

		glClearDepth( 1.0 );
		glClear( GL_DEPTH_BUFFER_BIT );
		glDepthRange( 0.0, 0.5 );
		glDepthFunc( GL_LESS );
	}
}

void Renderer::TuneAlphaThreshold( float alpha_threshold ) {
	m_alpha_threshold = alpha_threshold;
}

void Renderer::TunePrecomputeBlending( bool enable ) {
	if( !m_primitives.empty() ) {
#ifdef SFGUI_DEBUG
		std::cerr << "TunePrecomputeBlending() can only be called before any primitives are constructed.\n";
#endif
	}

	m_preblend = enable;
}

void Renderer::TuneCull( bool enable ) {
	m_cull = enable;
}

}