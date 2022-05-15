// Symphony of Empires
// Copyright (C) 2021, Symphony of Empires contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ----------------------------------------------------------------------------
// Name:
//      eng3d/ui/widget.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>
#include <string>
#include <cmath>
#include <algorithm>
#include <glm/vec2.hpp>

#include <GL/glew.h>
#include <GL/gl.h>

#include "eng3d/ui/widget.hpp"
#include "eng3d/ui/ui.hpp"
#include "eng3d/ui/checkbox.hpp"
#include "eng3d/path.hpp"
#include "eng3d/texture.hpp"
#include "eng3d/rectangle.hpp"
#include "eng3d/state.hpp"
#include "eng3d/ui/tooltip.hpp"
#include "eng3d/primitive.hpp"

#if !defined NOMINMAX
#   define NOMINMAX 1
#endif

using namespace UI;

Widget::Widget(Widget* _parent, int _x, int _y, const unsigned w, const unsigned h, WidgetType _type)
    : is_show{ 1 },
    type{ _type },
    x{ _x },
    y{ _y },
    width{ w },
    height{ h },
    parent{ _parent }
{
    if(parent != nullptr) {
        x += parent->padding.x;
        y += parent->padding.y;
        parent->add_child(this);
    }
    else {
        // Add the widget to the context in each construction without parent
        g_ui_context->add_widget(this);
    }
}

Widget::Widget(Widget* _parent, int _x, int _y, const unsigned w, const unsigned h, WidgetType _type, std::shared_ptr<Eng3D::Texture> tex)
    : is_show{ 1 },
    type{ _type },
    x{ _x },
    y{ _y },
    width{ w },
    height{ h },
    parent{ _parent },
    current_texture{ tex }
{
    if(parent != nullptr) {
        x += parent->padding.x;
        y += parent->padding.y;
        parent->add_child(this);
    } else {
        // Add the widget to the context in each construction without parent
        g_ui_context->add_widget(this);
    }
}

Widget::~Widget(void) {
    // Common texture also deleted?
    if(text_texture != nullptr) {
        delete text_texture;
    }

    if(tooltip != nullptr) {
        delete tooltip;
    }
}

void Widget::draw_rect(const Eng3D::Texture* tex, Eng3D::Rect rect_pos, Eng3D::Rect rect_tex, Eng3D::Rect viewport) {
    glm::vec2 pos_size = rect_pos.size();
    pos_size.x = pos_size.x > 0? pos_size.x : 1.f;
    pos_size.y = pos_size.y > 0? pos_size.y : 1.f;
    glm::vec2 tex_size = rect_tex.size();

    if(rect_pos.left < viewport.left) {
        float x_ratio = (viewport.left - rect_pos.left) / pos_size.x;
        rect_tex.left = rect_tex.left + x_ratio * tex_size.x;
        rect_pos.left = viewport.left;
    }
    if(rect_pos.right > viewport.right) {
        float x_ratio = (rect_pos.right - viewport.right) / pos_size.x;
        rect_tex.right = rect_tex.right - x_ratio * tex_size.x;
        rect_pos.right = viewport.right;
    }
    if(rect_pos.top < viewport.top) {
        float y_ratio = (viewport.top - rect_pos.top) / pos_size.y;
        rect_tex.top = rect_tex.top + y_ratio * tex_size.y;
        rect_pos.top = viewport.top;
    }
    if(rect_pos.bottom > viewport.bottom) {
        float y_ratio = (rect_pos.bottom - viewport.bottom) / pos_size.y;
        rect_tex.bottom = rect_tex.bottom - y_ratio * tex_size.y;
        rect_pos.bottom = viewport.bottom;
    }

    // TODO: rect_tex
    if(tex != nullptr) {
        g_ui_context->obj_shader->set_texture(0, "diffuse_map", *tex);
    }
    auto square = Eng3D::Square(rect_pos, rect_tex);
    square.draw();   
}

void Widget::above_of(const Widget& rhs) {
    y = rhs.y - height;
}

void Widget::below_of(const Widget& rhs) {
    y = rhs.y + rhs.height;
}

void Widget::left_side_of(const Widget& rhs) {
    x = rhs.x - width;
}

void Widget::right_side_of(const Widget& rhs) {
    x = rhs.x + rhs.width;
}

void Widget::draw_border(Border& border, Eng3D::Rect viewport) {
    float x_offset = border.offset.x;
    float y_offset = border.offset.y;
    float b_w = border.size.x;
    float b_h = border.size.y;
    float b_tex_w = border.texture_size.x;
    float b_tex_h = border.texture_size.y;

    const auto& border_tex = *border.texture.get();

    // Draw border edges and corners
    Eng3D::Rect pos_rect(0, 0, 0, 0);
    Eng3D::Rect tex_rect(0, 0, 0, 0);
    pos_rect.left = x_offset;
    pos_rect.top = y_offset;
    pos_rect.right = x_offset + b_w;
    pos_rect.bottom = y_offset + b_h;
    tex_rect.left = 0;
    tex_rect.top = 0;
    tex_rect.right = b_tex_w / border_tex.width;
    tex_rect.bottom = b_tex_h / border_tex.height;

    // Top left corner
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Top right corner
    pos_rect.left = width - b_w;
    tex_rect.left = (border_tex.width - b_tex_w) / border_tex.width;
    pos_rect.right = width;
    tex_rect.right = 1.f;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Bottom right corner
    pos_rect.top = height - b_h;
    tex_rect.top = (border_tex.height - b_tex_h) / border_tex.height;
    pos_rect.bottom = height;
    tex_rect.bottom = 1.f;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Bottom left corner
    pos_rect.left = x_offset;
    tex_rect.left = 0;
    pos_rect.right = x_offset + b_w;
    tex_rect.right = b_tex_w / border_tex.width;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Top edge
    pos_rect.left = x_offset + b_w;
    tex_rect.left = b_tex_w / border_tex.width;
    pos_rect.right = width - b_w;
    tex_rect.right = (border_tex.width - b_tex_w) / border_tex.width;
    pos_rect.top = y_offset;
    tex_rect.top = 0;
    pos_rect.bottom = y_offset + b_h;
    tex_rect.bottom = b_tex_h / border_tex.height;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Bottom edge
    pos_rect.top = height - b_h;
    tex_rect.top = (border_tex.height - b_tex_h) / border_tex.height;
    pos_rect.bottom = height;
    tex_rect.bottom = 1.f;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Left edge
    pos_rect.top = y_offset + b_h;
    tex_rect.top = b_tex_h / border_tex.height;
    pos_rect.bottom = height - b_h;
    tex_rect.bottom = (border_tex.height - b_tex_h) / border_tex.height;
    pos_rect.left = x_offset;
    tex_rect.left = 0;
    pos_rect.right = b_w;
    tex_rect.right = b_tex_w / border_tex.width;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Right edge
    pos_rect.left = width - b_w;
    tex_rect.left = (border_tex.width - b_tex_w) / border_tex.width;
    pos_rect.right = width;
    tex_rect.right = 1.f;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);

    // Middle
    pos_rect.left = x_offset + b_w;
    tex_rect.left = b_tex_w / border_tex.width;
    pos_rect.right = width - b_w;
    tex_rect.right = (border_tex.width - b_tex_w) / border_tex.width;
    pos_rect.top = y_offset + b_h;
    tex_rect.top = b_tex_h / border_tex.height;
    pos_rect.bottom = height - b_h;
    tex_rect.bottom = (border_tex.height - b_tex_h) / border_tex.height;
    draw_rect(&border_tex, pos_rect, tex_rect, viewport);
}

// Draw a simple quad
void Widget::draw_rectangle(int _x, int _y, unsigned _w, unsigned _h, Eng3D::Rect viewport, const Eng3D::Texture* tex) {
    // Texture switching in OpenGL is expensive
    Eng3D::Rect pos_rect(_x, _y, (int)_w, (int)_h);
    Eng3D::Rect tex_rect(0.f, 0.f, 1.f, 1.f);
    draw_rect(tex, pos_rect, tex_rect, viewport);
}

#include <deque>
void Widget::on_render(Context& ctx, Eng3D::Rect viewport) {
    g_ui_context->obj_shader->set_texture(0, "diffuse_map", *Eng3D::State::get_instance().tex_man->get_white());

    // Shadow
    if(have_shadow) {
        g_ui_context->obj_shader->set_uniform("diffuse_color", glm::vec4(0.f, 0.f, 0.7f, 1.f));
        auto square = Eng3D::Square(16.f, 16.f, width + 16.f, height + 16.f);
    }

    g_ui_context->obj_shader->set_uniform("diffuse_color", glm::vec4(1.f));
    // Background (tile) display
    if(type == UI::WidgetType::INPUT) {
        Eng3D::Rect pos_rect((int)0u, 0u, width, height);
        Eng3D::Rect tex_rect((int)0u, 0u, 1u, 1u);
        g_ui_context->obj_shader->set_uniform("diffuse_color", glm::vec4(0.f, 1.f, 0.f, 1.f));
        draw_rect(nullptr, pos_rect, tex_rect, viewport);
    }

    if(color.a != 0) {
        Eng3D::Rect pos_rect((int)0u, 0u, width, height);
        Eng3D::Rect tex_rect((int)0u, 0u, 1u, 1u);
        g_ui_context->obj_shader->set_uniform("diffuse_color", glm::vec4(color.r, color.g, color.b, color.a));
        draw_rect(nullptr, pos_rect, tex_rect, viewport);
    }

    g_ui_context->obj_shader->set_uniform("diffuse_color", glm::vec4(1.f));
    if(current_texture != nullptr) {
        draw_rectangle(0, 0, width, height, viewport, current_texture.get());
    }

    // Top bar of windows display
    if(type == UI::WidgetType::WINDOW) {
        draw_rectangle(0, 0, width, 24, viewport, ctx.window_top.get());
    }

    if(border.texture != nullptr) {
        draw_border(border, viewport);
    }
    
    if(text_texture != nullptr) {
        if(!text_texture->gl_tex_num) {
            text_texture->upload();
        }
        g_ui_context->obj_shader->set_uniform("diffuse_color", glm::vec4(text_color.r, text_color.g, text_color.b, 1.f));

        int x_offset = text_offset_x;
        int y_offset = text_offset_y;
        if(text_align_x == UI::Align::CENTER) {
            x_offset = (width - text_texture->width) / 2;
        } else if(text_align_x == UI::Align::END) {
            x_offset = width - text_texture->width;
        }

        if(text_align_y == UI::Align::CENTER) {
            y_offset = (height - text_texture->height) / 2;
        } else if(text_align_y == UI::Align::END) {
            y_offset = height - text_texture->height;
        }
        draw_rectangle(x_offset, y_offset, text_texture->width, text_texture->height, viewport, text_texture);
    }

    // Semi-transparent over hover elements which can be clicked
    if(clickable_effect && ((on_click && is_hover) || is_clickable)) {
        Eng3D::Rect pos_rect((int)0u, 0u, width, height);
        Eng3D::Rect tex_rect((int)0u, 0u, 1u, 1u);

        g_ui_context->obj_shader->set_texture(0, "diffuse_map", *Eng3D::State::get_instance().tex_man->get_white());
        g_ui_context->obj_shader->set_uniform("diffuse_color", glm::vec4(0.5f, 0.5f, 0.5f, 0.5f));
        draw_rect(nullptr, pos_rect, tex_rect, viewport);
    }
}

void Widget::move_by(int _x, int _y) {
    x += _x;
    y += _y;
}

void Widget::recalc_child_pos() {
    if(flex == Flex::NONE) {
        return;
    }

    bool is_row = flex == Flex::ROW;
    size_t lenght = 0;
    int movable_children = 0;
    for(auto& child : children) {
        if(!child->is_pinned) {
            lenght += is_row ? child->width : child->height;
            movable_children++;
        }
    }

    size_t current_lenght = 0;
    int size = 0, difference = 0;
    switch(flex_justify) {
    case FlexJustify::START:
        current_lenght = 0;
        for(auto& child : children) {
            if(child->is_pinned) continue;
            if(is_row) {
                child->x = current_lenght;
                current_lenght += child->width + flex_gap;
            }
            else {
                child->y = current_lenght;
                current_lenght += child->height + flex_gap;
            }
        }
        break;
    case FlexJustify::END:
        current_lenght = is_row ? width : height;
        for(int i = children.size() - 1; i >= 0; i--) {
            auto& child = children[i];
            if(child->is_pinned) continue;
            if(is_row) {
                child->x = current_lenght - child->width - flex_gap;
                current_lenght -= child->width;
            }
            else {
                child->y = current_lenght - child->height - flex_gap;
                current_lenght -= child->height;
            }
        }
        break;
    case FlexJustify::SPACE_BETWEEN:
        current_lenght = 0;
        size = is_row ? width : height;
        difference = (size - lenght) / (std::max(1, movable_children - 1));
        for(auto& child : children) {
            if(child->is_pinned) continue;
            if(is_row) {
                child->x = current_lenght;
                current_lenght += child->width + difference;
            }
            else {
                child->y = current_lenght;
                current_lenght += child->height + difference;
            }
        }
        break;
    case FlexJustify::SPACE_AROUND:
        size = is_row ? width : height;
        difference = (size - lenght) / movable_children;
        current_lenght = std::max<int>(0, difference / 2);
        for(auto& child : children) {
            if(child->is_pinned) continue;
            if(is_row) {
                child->x = current_lenght;
                current_lenght += child->width + difference;
            }
            else {
                child->y = current_lenght;
                current_lenght += child->height + difference;
            }
        }
        break;
    }

    switch(flex_align) {
    case Align::START:
        for(auto& child : children) {
            if(child->is_pinned) continue;
            if(is_row) {
                child->y = 0;
            }
            else {
                child->x = 0;
            }
        }
        break;
    case Align::END:
        for(auto& child : children) {
            if(child->is_pinned) continue;
            if(is_row) {
                child->y = std::max<int>(0, height - child->height);
            }
            else {
                child->x = std::max<int>(0, width - child->width);
            }
        }
        break;
    case Align::CENTER:
        for(auto& child : children) {
            if(child->is_pinned) continue;
            if(is_row) {
                child->y = std::max<int>(0, height - child->height) / 2;
            }
            else {
                child->x = std::max<int>(0, width - child->width) / 2;
            }
        }
        break;
    }
    int child_index = 0;
    for(auto& child : children) {
        if(!child->is_pinned) {
            if(child->on_pos_recalc) {
                child->on_pos_recalc(*child, child_index);
            }
            child_index++;
        }
    }
}

void Widget::add_child(UI::Widget* child) {
    // Add to list
    children.push_back(std::move(std::unique_ptr<UI::Widget>(child)));
    child->parent = this;

    // Child changes means a recalculation of positions is in order
    need_recalc = true;
}

static inline unsigned int power_two_floor(const unsigned int val) {
    unsigned int power = 2, nextVal = power * 2;
    while((nextVal *= 2) <= val) {
        power *= 2;
    }
    return power * 2;
}

void Widget::text(const std::string& _text) {
    if(this->text_str == _text) {
        return;
    }
    text_str = _text;

    // Copy _text to a local scope (SDL2 does not like references)
    if(text_texture != nullptr) {
        // Auto deletes gl_texture
        delete text_texture;
        text_texture = nullptr;
    }

    if(_text.empty()) {
        return;
    }

    TTF_Font* text_font = font ? font : g_ui_context->default_font;
    text_texture = new Eng3D::Texture(text_font, text_color, _text);
}

void Widget::set_tooltip(UI::Tooltip* _tooltip) {
    this->tooltip = _tooltip;
    this->tooltip->parent = this;
}

void Widget::set_tooltip(const std::string& text) {
    if(text.empty()) {
        return;
    }
    this->set_tooltip(new UI::Tooltip(this, std::min<unsigned int>(text.size() * 12, 512), ((text.size() * 12) / 512) * 24));
    this->tooltip->text(text);
}

void Widget::scroll(int y) {
    int child_top = 0;
    int child_bottom = height;
    for(auto& child : children) {
        if(!child->is_pinned) {
            child_top = std::min(child_top, child->y);
            child_bottom = std::max(child_bottom, child->y + (int)child->height);
        }
    }
    child_bottom -= height;
    y = std::min(-child_top, y);
    y = std::max(-child_bottom, y);

    for(auto& child : children) {
        if(!child->is_pinned) {
            child->y += y;
        }
    }
}