#include "font.h"
#include "director.h"
#include "util.h"

Font::Font(const std::string& path, uint32_t char_size)
: _path{path}, _char_size{char_size}, _font{new sf::Font}
{
  if (!path.empty()) {
    _font->loadFromFile(path);
  }
}

void Font::bind_texture() const
{
  sf::Texture::bind(&_font->getTexture(_char_size));
}

const std::string& Font::get_path() const
{
  return _path;
}

sf::Vector2f Font::get_size(const std::string& text) const
{
  const auto& entry = compute_size(text);
  return {entry.max.x - entry.min.x, entry.max.y - entry.min.y};
}

std::vector<Font::vertex> Font::get_vertices(const std::string& text) const
{
  const auto& entry = compute_size(text);
  const auto& texture = _font->getTexture(_char_size);

  auto hspace = _font->getGlyph(' ', _char_size, false).advance;
  auto vspace = _font->getLineSpacing(_char_size);
  sf::Vector2f pos;

  std::vector<Font::vertex> vertices;
  uint32_t prev = 0;
  for (char current : text) {
    pos.x += _font->getKerning(prev, current, _char_size);
    prev = current;

    switch (current) {
    case ' ':
      pos.x += hspace;
      continue;
    case '\t':
      pos.x += hspace * 4;
      continue;
    case '\n':
      pos.y += vspace;
      pos.x = 0;
      continue;
    case '\v':
      pos.y += vspace * 4;
      continue;
    }

    const auto& g = _font->getGlyph(current, _char_size, false);
    float x1 = pos.x + g.bounds.left;
    float y1 = pos.y + g.bounds.top;
    float x2 = pos.x + g.bounds.left + g.bounds.width;
    float y2 = pos.y + g.bounds.top + g.bounds.height;
    float u1 = float(g.textureRect.left) / texture.getSize().x;
    float v1 = float(g.textureRect.top) / texture.getSize().y;
    float u2 = float(g.textureRect.left + g.textureRect.width) / texture.getSize().x;
    float v2 = float(g.textureRect.top + g.textureRect.height) / texture.getSize().y;

    vertices.push_back({x1, y1, u1, v1});
    vertices.push_back({x2, y1, u2, v1});
    vertices.push_back({x2, y2, u2, v2});
    vertices.push_back({x1, y2, u1, v2});
    pos.x += g.advance;
  }
  for (auto& v : vertices) {
    v.x -= entry.min.x + (entry.max.x - entry.min.x) / 2;
    v.y -= entry.min.y + (entry.max.y - entry.min.y) / 2;
  }
  return vertices;
}

Font::rectangle Font::compute_size(const std::string& text) const
{
  auto hspace = _font->getGlyph(' ', _char_size, false).advance;
  auto vspace = _font->getLineSpacing(_char_size);

  sf::Vector2f pos;
  sf::Vector2f min = {256.f, 256.f};
  sf::Vector2f max = {-256.f, -256.f};

  uint32_t prev = 0;
  for (char current : text) {
    pos.x += _font->getKerning(prev, current, _char_size);
    prev = current;

    switch (current) {
    case ' ':
      pos.x += hspace;
      continue;
    case '\t':
      pos.x += hspace * 4;
      continue;
    case '\n':
      pos.y += vspace;
      pos.x = 0;
      continue;
    case '\v':
      pos.y += vspace * 4;
      continue;
    }

    const auto& g = _font->getGlyph(current, _char_size, false);
    min.x = std::min(min.x, pos.x + g.bounds.left);
    max.x = std::max(max.x, pos.x + g.bounds.left + g.bounds.width);
    min.y = std::min(min.y, pos.y + g.bounds.top);
    max.y = std::max(max.y, pos.y + g.bounds.top + g.bounds.height);
    pos.x += g.advance;
  }
  return rectangle{min, max};
}

FontCache::FontCache(const std::string& root_path, uint32_t char_size, uint32_t font_cache_size)
: _root_path{root_path}, _char_size{char_size}, _font_cache_size{font_cache_size}
{
}

const Font& FontCache::get_font(const std::string& font_path) const
{
  auto full_path = _root_path + "/" + font_path;

  auto it = _map.find(full_path);
  if (it != _map.end()) {
    auto jt = _list.begin();
    while (&*jt != it->second) {
      ++jt;
    }
    _list.splice(_list.begin(), _list, jt);
    return *it->second;
  }

  _list.emplace_front(full_path, _char_size);
  _map.emplace(full_path, &_list.front());
  if (_list.size() > _font_cache_size) {
    _map.erase(_list.back().get_path());
    _list.pop_back();
  }
  return _list.front();
}