#include "View.hpp"

View::View() {
    m_viewport.x = 0;
    m_viewport.y = 0;
    m_viewport.w = 0;
    m_viewport.h = 0;
}

void View::setCenter(float x, float y) {
    m_viewport.x = x - m_viewport.w / 2;
    m_viewport.y = y - m_viewport.h / 2;
}

void View::setSize(float width, float height) {
    m_viewport.w = width;
    m_viewport.h = height;
}

const SDL_FRect& View::getViewport() const {
    return m_viewport;
}
