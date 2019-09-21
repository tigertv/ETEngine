#include "stdafx.h"
#include "TextRenderer.h"

#include <glad/glad.h>

#include <EtCore/Content/ResourceManager.h>

#include <Engine/Graphics/SpriteFont.h>
#include <Engine/Graphics/Shader.h>
#include <Engine/Graphics/TextureData.h>


//=============================
// Text Renderer :: Text Cache
//=============================


//---------------------------------
// TextRenderer::TextCache::c-tor
//
// Initialize a text cache
//
TextRenderer::TextCache::TextCache(std::string const& text, vec2 const pos, vec4 const& col, int16 const size)
	: Text(text)
	, Position(pos)
	, Color(col)
	, Size(size)
{ }


//===============
// Text Renderer
//===============


//---------------------------------
// TextRenderer::d-tor
//
// Delete GPU objects
//
TextRenderer::~TextRenderer()
{
	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	api->DeleteVertexArrays(1, &m_VAO);
	api->DeleteBuffers(1, &m_VBO);
}

//---------------------------------
// TextRenderer::Initialize
//
// Create handles to GPU objects
//
void TextRenderer::Initialize()
{
	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	m_pTextShader = ResourceManager::Instance()->GetAssetData<ShaderData>("PostText.glsl"_hash);

	api->SetShader(m_pTextShader.get());

	m_pTextShader->Upload("fontTex"_hash, 0);

	//Generate buffers and arrays
	api->GenerateVertexArrays(1, &m_VAO);
	api->GenerateBuffers(1, &m_VBO);


	//bind
	api->BindVertexArray(m_VAO);
	api->BindBuffer(GL_ARRAY_BUFFER, m_VBO);

	//set data and attributes
	api->SetBufferData(GL_ARRAY_BUFFER, m_BufferSize, NULL, GL_DYNAMIC_DRAW);

	//input layout

	api->SetVertexAttributeArrayEnabled(0, true);
	api->SetVertexAttributeArrayEnabled(1, true);
	api->SetVertexAttributeArrayEnabled(2, true);
	api->SetVertexAttributeArrayEnabled(3, true);
	api->SetVertexAttributeArrayEnabled(4, true);
	api->SetVertexAttributeArrayEnabled(5, true);

	int32 const vertSize = sizeof(TextVertex);
	api->DefineVertexAttributePointer(0, (GLint)3, GL_FLOAT, GL_FALSE, vertSize, (GLvoid*)offsetof(TextVertex, Position));
	api->DefineVertexAttributePointer(1, (GLint)4, GL_FLOAT, GL_FALSE, vertSize, (GLvoid*)offsetof(TextVertex, Color));
	api->DefineVertexAttributePointer(2, (GLint)2, GL_FLOAT, GL_FALSE, vertSize, (GLvoid*)offsetof(TextVertex, TexCoord));
	api->DefineVertexAttributePointer(3, (GLint)2, GL_FLOAT, GL_FALSE, vertSize, (GLvoid*)offsetof(TextVertex, CharacterDimension));
	api->DefineVertexAttributePointer(4, (GLint)1, GL_FLOAT, GL_FALSE, vertSize, (GLvoid*)offsetof(TextVertex, SizeMult));
	api->DefineVertexAttribIPointer(5, (GLint)1, GL_UNSIGNED_INT, vertSize, (GLvoid*)offsetof(TextVertex, ChannelId));

	//unbind
	api->BindBuffer(GL_ARRAY_BUFFER, 0);
	api->BindVertexArray(0);

	CalculateTransform();

	Config::GetInstance()->GetWindow().WindowResizeEvent.AddListener( std::bind( &TextRenderer::OnWindowResize, this ) );
}

//---------------------------------
// TextRenderer::SetFont
//
// Sets the active font and adds it to the queue if it's not there yet
//
void TextRenderer::SetFont(SpriteFont const* const font)
{
	auto foundIt = std::find_if(m_QueuedFonts.begin(), m_QueuedFonts.end(), [font](QueuedFont const& queued)
		{
			return queued.m_Font == font;
		});

	if (foundIt == m_QueuedFonts.cend())
	{
		m_ActiveFontIdx = m_QueuedFonts.size();
		m_QueuedFonts.emplace_back(QueuedFont());
		m_QueuedFonts[m_ActiveFontIdx].m_Font = font;
	}
	else
	{
		m_ActiveFontIdx = foundIt - m_QueuedFonts.begin();
	}
}

//---------------------------------
// TextRenderer::DrawText
//
// Creates a text cache and adds it to the active font
//
void TextRenderer::DrawText(std::string const& text, vec2 const pos, int16 fontSize)
{
	ET_ASSERT(m_QueuedFonts.size() > 0u, "No active font set!");

	if (fontSize <= 0)
	{
		fontSize = m_QueuedFonts[m_ActiveFontIdx].m_Font->GetFontSize();
	}

	m_NumCharacters += static_cast<uint32>(text.size());
	m_QueuedFonts[m_ActiveFontIdx].m_TextCache.emplace_back(TextCache(text, pos, m_Color, fontSize));
	m_QueuedFonts[m_ActiveFontIdx].m_IsAddedToRenderer = true;
}

//---------------------------------
// TextRenderer::OnWindowResize
//
// Makes sure text size stays consistent when the window size changes
//
void TextRenderer::OnWindowResize()
{
	CalculateTransform();
}

//---------------------------------
// TextRenderer::GetTextSize
//
// Returns the theoretical dimensions of a text that would be drawn
//
ivec2 TextRenderer::GetTextSize(std::string const& text, SpriteFont const* const font, int16 fontSize) const
{
	if (fontSize <= 0)
	{
		fontSize = font->GetFontSize();
	}

	float sizeMult = (float)fontSize / (float)font->GetFontSize();
	vec2 ret(0.f);

	for (auto charId : text)
	{
		char previous = 0;
		if (SpriteFont::IsCharValid(charId) && font->GetMetric(charId).IsValid)
		{
			FontMetric const& metric = font->GetMetric(charId);

			vec2 kerningVec = 0;
			if (font->m_UseKerning)
			{
				kerningVec = metric.GetKerningVec(static_cast<wchar_t>(previous));
			}
			previous = charId;

			ret.x += (metric.AdvanceX + kerningVec.x) * sizeMult;

			if (charId == ' ')
			{
				continue;
			}

			ret.y = std::max(ret.y, (static_cast<float>(metric.Height) + static_cast<float>(metric.OffsetY) + kerningVec.y) * sizeMult);
		}
		else
		{
			LOG("TextRenderer::GetTextSize>char not supported for current font", Warning);
		}
	}

	return etm::vecCast<int32>(ret);
}

//---------------------------------
// TextRenderer::Draw
//
// Draws all currently queued fonts to the active render target
//
void TextRenderer::Draw()
{
	if (m_QueuedFonts.size() <= 0)
	{
		return;
	}

	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	//Bind Object vertex array
	api->BindVertexArray(m_VAO);

	UpdateBuffer();

	if (INPUT->GetKeyState(E_KbdKey::K) == E_KeyState::Pressed)
	{
		m_bUseKerning = !m_bUseKerning;
	}

	//Enable this objects shader
	CalculateTransform();
	api->SetShader(m_pTextShader.get());
	api->SetActiveTexture(0);
	m_pTextShader->Upload("transform"_hash, m_Transform);

	//Bind Object vertex array
	api->BindVertexArray(m_VAO);

	for (QueuedFont& queued : m_QueuedFonts)
	{
		if (queued.m_IsAddedToRenderer)
		{
			api->BindTexture(GL_TEXTURE_2D, queued.m_Font->GetAtlas()->GetHandle());

			vec2 const texSize = etm::vecCast<float>(queued.m_Font->GetAtlas()->GetResolution());
			m_pTextShader->Upload("texSize"_hash, texSize);

			//Draw the object
			api->DrawArrays(GL_POINTS, queued.m_BufferStart, queued.m_BufferSize);

			queued.m_IsAddedToRenderer = false;
		}
	}

	//unbind vertex array
	api->BindVertexArray(0);
}

//---------------------------------
// TextRenderer::UpdateBuffer
//
// Updates the vertex buffer containing font vertices
//
void TextRenderer::UpdateBuffer()
{
	std::vector<TextVertex> tVerts;

	for (QueuedFont& queued : m_QueuedFonts)
	{
		if (queued.m_IsAddedToRenderer)
		{
			queued.m_BufferStart = static_cast<int32>(tVerts.size() * (sizeof(TextVertex) / sizeof(float)));
			queued.m_BufferSize = 0;

			for (TextCache const& cache : queued.m_TextCache)
			{
				float const sizeMult = static_cast<float>(cache.Size) / static_cast<float>(queued.m_Font->GetFontSize());

				float totalAdvanceX = 0.f;
				char previous = 0;
				for (char const charId : cache.Text)
				{
					if (!SpriteFont::IsCharValid(charId))
					{
						LOG(FS("TextRenderer::UpdateBuffer > char '%c' not supported for current font", charId), Warning);
						continue;
					}

					FontMetric const& metric = queued.m_Font->GetMetric(charId);
					if (!metric.IsValid)
					{
						LOG(FS("TextRenderer::UpdateBuffer > char '%c' doesn't have a valid metric", charId), Warning);
						continue;
					}

					vec2 kerningVec = 0;
					if (queued.m_Font->m_UseKerning && m_bUseKerning)
					{
						kerningVec = metric.GetKerningVec(static_cast<wchar_t>(previous)) * sizeMult;
					}
					previous = charId;

					totalAdvanceX += kerningVec.x;

					if (charId == ' ')
					{
						totalAdvanceX += metric.AdvanceX;
						continue;
					}

					tVerts.push_back(TextVertex());
					TextVertex& vText = tVerts[tVerts.size()-1];

					vText.Position.x = cache.Position.x + (totalAdvanceX + metric.OffsetX)*sizeMult;
					vText.Position.y = cache.Position.y + (kerningVec.y + metric.OffsetY)*sizeMult;
					vText.Position.z = 0;
					vText.Color = cache.Color;
					vText.TexCoord = metric.TexCoord;
					vText.CharacterDimension = vec2(metric.Width, metric.Height);
					vText.SizeMult = sizeMult;
					vText.ChannelId = metric.Channel;

					totalAdvanceX += metric.AdvanceX;
				}
			}

			queued.m_BufferSize = static_cast<int32>(tVerts.size()) * (sizeof(TextVertex) / sizeof(float)) - queued.m_BufferStart;
			queued.m_TextCache.clear();
		}
	}

	GraphicsApiContext* const api = Viewport::GetCurrentApiContext();

	//Bind Object vertex array
	api->BindVertexArray(m_VAO);

	//Send the vertex buffer again
	api->BindBuffer(GL_ARRAY_BUFFER, m_VBO);
	api->SetBufferData(GL_ARRAY_BUFFER, static_cast<uint32>(tVerts.size() * sizeof(TextVertex)), tVerts.data(), GL_DYNAMIC_DRAW);
	api->BindBuffer(GL_ARRAY_BUFFER, 0);

	//Done Modifying
	api->BindVertexArray(0);

	m_NumCharacters = 0;
}

//---------------------------------
// TextRenderer::CalculateTransform
//
// Calculates the transformation matrix to be used by the GPU to position sprites
//
void TextRenderer::CalculateTransform()
{
	ivec2 viewPos, viewSize;
	Viewport::GetCurrentApiContext()->GetViewport(viewPos, viewSize);
	int32 width = viewSize.x, height = viewSize.y;
	float scaleX = (width > 0) ? 2.f / width : 0;
	float scaleY = (height > 0) ? 2.f / height : 0;

	m_Transform = mat4({
		scaleX,	0,			0,		0,
		0,		-scaleY,	0,		0,
		0,		0,			1,		0,
		-1,		1,			0,		1 });
}
