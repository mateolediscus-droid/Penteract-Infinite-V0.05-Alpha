#include "ui.h"
#include <glm/gtc/random.hpp>
#include <cassert>

#include "utils.h"

ui::page::page() : currentCursor{ glfwCreateStandardCursor(currentCursorType) }
{
}

GLFWwindow* ui::page::getGlfwWindow()
{
	return window;
}

void ui::page::getCursorPos(double* xpos, double* ypos)
{
	utils::getCursorPos(window, xpos, ypos);
}

void ui::page::changeViewport(const glm::ivec4& pos, const glm::ivec2& scroll)
{
	int wWidth, wHeight;
	glfwGetFramebufferSize(window, &wWidth, &wHeight);

	if (wWidth == 0 || wHeight == 0) return;

	glViewport(pos.x, wHeight - pos.y - pos.w, pos.z, pos.w);

	glm::mat4 projection2D = glm::ortho(0.0f, static_cast<float>(pos.z),
		static_cast<float>(pos.w), 0.0f, -1.0f, 1.0f);
	projection2D = glm::translate(projection2D, { scroll.x, scroll.y, 0 });

	// update the projection matrices for all of the shaders
	Shader::get("quad")->setUniform("P", projection2D);
	Shader::get("text")->setUniform("P", projection2D);
	Shader::get("tex")->setUniform("proj", projection2D);

	viewportUpdated = true;
}

void ui::page::addElem(element* e)
{
	elems.push_back(e);

	if (window != nullptr)
	{
		e->init(this);
	}
}

bool ui::page::removeElem(element* e)
{
	for (auto it = elems.begin(); it != elems.end(); ++it)
	{
		if (*it == e)
		{
			// don't remove the element directly so it can be processed correctly
			removeElem(it);
			return true;
		}
	}

	return false;
}

void ui::page::removeElem(container_t::iterator it)
{
	// must get index from iterator before erasing
	int index = it - elems.begin();
	if (index == focusIndex)
	{
		focusIndex = -1;
	}
	else if (index < focusIndex)
	{
		--focusIndex;
	}

	if (index == mouseDownIndex)
	{
		mouseDownIndex = -1;
	}
	else if (index < mouseDownIndex)
	{
		--mouseDownIndex;
	}

	elems.erase(it);
}

void ui::page::clear()
{
	focusIndex = -1;
	mouseDownIndex = -1;
	elems.clear();
}

ui::page::container_t::iterator ui::page::begin()
{
	return elems.begin();
}

ui::page::container_t::iterator ui::page::end()
{
	return elems.end();
}

ui::page::container_t::const_iterator ui::page::begin() const
{
	return elems.begin();
}

ui::page::container_t::const_iterator ui::page::end() const
{
	return elems.end();
}

void ui::page::init(GLFWwindow* w)
{
	window = w;

	int wWidth, wHeight;
	glfwGetFramebufferSize(window, &wWidth, &wHeight);
	windowResize(wWidth, wHeight);

	for (auto elem : elems)
	{
		elem->init(this);
	}
}

void ui::page::render()
{
	int wWidth, wHeight;
	glfwGetFramebufferSize(window, &wWidth, &wHeight);

	for (auto elem : elems)
	{
		elem->render(this);
		if (viewportUpdated)
		{
			changeViewport({ 0, 0, wWidth, wHeight }, {});
			viewportUpdated = false;
		}
	}
}

void ui::page::mouseInput(double xpos, double ypos)
{
	int mx = xpos;
	int my = ypos;

	int cursorType = GLFW_ARROW_CURSOR;

	if (element* focusedElem = getFocusedElem();
		focusedElem != nullptr && focusedElem->mouseInput(this, xpos, ypos))
	{
		cursorType = focusedElem->getCursorType();
	}
	else
	{
		// iterate in reverse order
		for (auto it = elems.rbegin(); it != elems.rend(); ++it)
		{
			element* elem = *it;

			int x, y, w, h;
			elem->getBounds(this, &x, &y, &w, &h);
			if (mx < x || my < y || mx >= x + w || my >= y + h)
			{
				continue;
			}

			if ((*it)->mouseInput(this, xpos, ypos))
			{
				cursorType = elem->getCursorType();
				break;
			}
		}
	}

	if (cursorType != currentCursorType)
	{
		glfwDestroyCursor(currentCursor);
		currentCursorType = cursorType;
		currentCursor = glfwCreateStandardCursor(cursorType);
		glfwSetCursor(window, currentCursor);
	}
}

bool ui::page::scrollInput(double xoff, double yoff)
{
	double xpos, ypos;
	int mx, my;
	utils::getCursorPos(window, &xpos, &ypos);
	mx = xpos;
	my = ypos;

	for (auto elem : elems)
	{
		if (!elem->clickable())
		{
			continue;
		}

		int x, y, w, h;
		elem->getBounds(this, &x, &y, &w, &h);
		if (mx < x || mx < y || mx >= x + w || mx >= y + h)
		{
			continue;
		}

		if (elem->scrollInput(this, xoff, yoff))
		{
			return true;
		}
	}

	return false;
}

bool ui::page::mouseButtonInput(int button, int action, int mods)
{
	double xpos, ypos;
	int mx, my;
	utils::getCursorPos(window, &xpos, &ypos);
	mx = xpos;
	my = ypos;

	// use indices in case an element callback modifies the contents of this page
	for (int i = 0; i < elems.size(); ++i)
	{
		element* elem = elems[i];
		if (!elem->clickable())
		{
			continue;
		}

		int x, y, w, h;
		elem->getBounds(this, &x, &y, &w, &h);
		if (mx < x || my < y || mx >= x + w || my >= y + h)
		{
			continue;
		}

		if (elem->mouseButtonInput(this, xpos, ypos, button, action, mods))
		{
			if (element* focusElem = getFocusedElem();
				elem != focusElem)
			{
				if (focusElem != nullptr)
				{
					focusElem->defocus();
				}

				elem->focus();
			}

			focusIndex = i;

			if (mouseDownIndex != focusIndex)
			{
				mouseDownCancel();
			}
			mouseDownIndex = focusIndex;
			return true;
		}
	}

	if (action == GLFW_PRESS)
	{
		if (element* focusElem = getFocusedElem();
			focusElem != nullptr)
		{
			focusElem->defocus();
			focusIndex = -1;
		}
	}
	mouseDownCancel();
	return false;
}

bool ui::page::keyInput(int key, int scancode, int action, int mods)
{
	element* elem = getFocusedElem();
	if (elem == nullptr)
	{
		return false;
	}

	return elem->keyInput(this, key, scancode, action, mods);
}

void ui::page::windowResize(int width, int height)
{
	changeViewport({ 0, 0, width, height }, {});

	for (auto elem : elems)
	{
		elem->windowResize(this, width, height);
	}
}

bool ui::page::charInput(uint32_t codepoint)
{
	element* elem = getFocusedElem();
	if (elem == nullptr)
	{
		return false;
	}

	return elem->charInput(this, codepoint);
}

bool ui::page::fileDrop(int count, const char* paths[])
{
	element* elem = getFocusedElem();
	if (elem == nullptr)
	{
		return false;
	}

	return elem->fileDrop(this, count, paths);
}

ui::element* ui::page::getFocusedElem()
{
	assert(focusIndex < 0 || focusIndex < elems.size());
	if (focusIndex > 0)
	{
		return elems[focusIndex];
	}
	return nullptr;
}

ui::element* ui::page::getMouseDownElem()
{
	assert(mouseDownIndex < 0 || mouseDownIndex < elems.size());
	if (mouseDownIndex > 0)
	{
		return elems[mouseDownIndex];
	}
	return nullptr;
}

void ui::page::mouseDownCancel()
{
	if (mouseDownIndex < 0)
	{
		return;
	}

	assert(mouseDownIndex < elems.size());
	elems[mouseDownIndex]->mouseDownCancel();

	mouseDownIndex = -1;
}

QuadRenderer ui::element::qr;
TextRenderer ui::element::tr;
void ui::element::renderInit()
{
	qr.setShader(Shader::get("quad"));
	qr.init();

	Texture::load("assets/textures/pixelfont.png", 0, false, "pixelfont.png");
	tr.setShader(Shader::get("text"));
	tr.setTexture(Texture::get("pixelfont.png"));
	tr.init();

	checkbox::renderInit();
}

ui::alignx ui::element::getAlignX() const
{
	return alignmentX;
}

ui::aligny ui::element::getAlignY() const
{
	return alignmentY;
}

void ui::element::setAlignX(alignx alignment)
{
	alignmentX = alignment;
}

void ui::element::setAlignY(aligny alignment)
{
	alignmentY = alignment;
}

int ui::element::getOffsetX() const
{
	return offsetX;
}

int ui::element::getOffsetY() const
{
	return offsetY;
}

void ui::element::setOffsetX(int offset)
{
	offsetX = offset;
}

void ui::element::setOffsetY(int offset)
{
	offsetY = offset;
}

void ui::text::getBounds(window* win, int* x, int* y, int* w, int* h)
{
	getBoundSize(w, h);

	int wWidth, wHeight;
	glfwGetFramebufferSize(win->getGlfwWindow(), &wWidth, &wHeight);

	switch (alignmentX)
	{
	case ALIGN_LEFT:
		*x = 0 + offsetX;
		break;
	case ALIGN_RIGHT:
		*x = wWidth - *w + offsetX;
		break;
	case ALIGN_CENTER_X:
	default:
		*x = (wWidth / 2) - (*w / 2) + offsetX;
		break;
	}

	switch (alignmentY)
	{
	case ALIGN_TOP:
		*y = 0 + offsetY;
		break;
	case ALIGN_BOTTOM:
		*y = wHeight - *h + offsetY;
		break;
	case ALIGN_CENTER_Y:
	default:
		*y = (wHeight / 2) - (*h / 2) + offsetY;
		break;
	}
}

void ui::text::render(window* win)
{
	// copied from a really old version of 4dm (for the most part) lol
	int x, y, w, h;
	getBounds(win, &x, &y, &w, &h);

	int charWidth = tr.getCharWidth() * size;
	int charHeight = tr.getCharHeight() * size * 1.5;

	if (wrapWidth == 0)
	{
		size_t yOffset = 0;
		size_t lineStart = 0;
		size_t pos = 0;
		for (;;)
		{
			pos = glm::min(text.find('\n', pos), text.length());
			renderText(win, text.substr(lineStart, pos - lineStart), x, y + yOffset * charHeight, false);

			if (pos == text.length())
			{
				break;
			}

			lineStart = pos;
			++pos;
			++yOffset;
		}
		return;
	}

	if (alignmentX == ALIGN_CENTER_X)
	{
		x += wrapWidth / 2;
	}
	else if (alignmentX == ALIGN_RIGHT)
	{
		x += wrapWidth;
	}

	size_t yOffset = 0;
	size_t lineStart = 0;
	size_t lastWordEnd = 0;
	size_t wordEnd = 0;
	size_t pos = 0;
	while (pos + 1 < text.length())
	{
		pos = glm::min(text.find(' ', pos), text.find('\n', pos));

		if (pos == std::string::npos)
		{
			pos = text.length() - 1;
		}

		lastWordEnd = wordEnd;
		wordEnd = pos;
		++pos;

		if (text[lastWordEnd] == '\n' || (wordEnd - lineStart) * charWidth > wrapWidth)
		{
			size_t lineEnd = (lastWordEnd == lineStart ? wordEnd : lastWordEnd);
			renderText(win, text.substr(lineStart, lineEnd - lineStart), x, y + yOffset * charHeight, true);
			lineStart = lineEnd + 1;
			lastWordEnd = lineStart;
			wordEnd = lineStart;
			pos = lineStart;
			++yOffset;
		}
	}

	// render any remaining text
	renderText(win, text.substr(lineStart), x, y + yOffset * charHeight, true);
}

void ui::text::setText(std::string_view value)
{
	text = value;
}

int ui::text::getSize() const
{
	return this->size;
}
void ui::text::setSize(uint32_t size)
{
	this->size = size;
}

void ui::text::setWrapWidth(uint32_t wrapWidth)
{
	this->wrapWidth = wrapWidth;
}

void ui::text::setColor(const glm::vec4& color)
{
	this->color = color;
}

void ui::text::setShadow(bool shadow)
{
	this->shadow = shadow;
}

void ui::text::getBoundSize(int* w, int* h)
{
	int charWidth = tr.getCharWidth() * size;
	int charHeight = tr.getCharHeight() * size * 1.5;
	if (wrapWidth == 0)
	{
		// count the lines
		size_t maxLineLength = 0;
		size_t lineCount = 1;
		size_t lineStart = 0;
		size_t pos = 0;
		for (;;)
		{
			pos = glm::min(text.find('\n', pos), text.length());
			maxLineLength = glm::max(pos - lineStart, maxLineLength);

			if (pos == text.length())
			{
				break;
			}

			lineStart = pos;
			++pos;
			++lineCount;
		}

		*w = maxLineLength * charWidth;
		*h = lineCount * charHeight;

		return;
	}

	*w = wrapWidth;

	if (text.length() * charWidth <= wrapWidth && !text.contains('\n'))
	{
		*h = charHeight;
		return;
	}

	// calculate height
	size_t yOffset = 0;
	size_t lineStart = 0;
	size_t lastWordEnd = 0;
	size_t wordEnd = 0;
	size_t pos = 0;
	while (pos < text.length() - 1)
	{
		pos = glm::min(text.find(' ', pos), text.find('\n', pos));

		if (pos == std::string::npos)
		{
			pos = text.length() - 1;
		}

		lastWordEnd = wordEnd;
		wordEnd = pos;
		++pos;

		if (text[lastWordEnd] == '\n' || (wordEnd - lineStart) * charWidth > wrapWidth)
		{
			size_t lineEnd = (lastWordEnd == lineStart ? wordEnd : lastWordEnd);
			lineStart = lineEnd + 1;
			lastWordEnd = lineStart;
			wordEnd = lineStart;
			pos = lineStart;
			++yOffset;
		}
	}

	*h = (yOffset + 1) * charHeight;
}

void ui::text::renderText(window* win, std::string_view text, int x, int y, bool align)
{
	if (text.empty())
	{
		return;
	}

	while (text[0] == '\n' || text[0] == '\r')
	{
		text = text.substr(1);

		if (text.empty())
		{
			return;
		}
	}

	tr.setText(text);
	tr.align(false);
	tr.setFontSize(size);

	int adjustedX;
	int charWidth = tr.getCharWidth() * size;
	if (!align || alignmentX == ALIGN_LEFT)
	{
		adjustedX = x;
	}
	else if (alignmentX == ALIGN_CENTER_X)
	{
		adjustedX = x - (charWidth * text.length() / 2);
	}
	else
	{
		adjustedX = x - charWidth * text.length();
	}

	if (shadow)
	{
		tr.setColor(0, 0, 0);
		tr.setPos(adjustedX + size, y + size);
		tr.render();
	}

	tr.setColor(color);
	tr.setPos(adjustedX, y);
	tr.render();
}

void ui::image::getBounds(window* win, int* x, int* y, int* w, int* h)
{
	*w = width;
	*h = height;

	int wWidth, wHeight;
	glfwGetFramebufferSize(win->getGlfwWindow(), &wWidth, &wHeight);

	switch (alignmentX)
	{
	case ALIGN_LEFT:
	{
		*x = 0 + offsetX;
		break;
	}
	case ALIGN_RIGHT:
	{
		*x = wWidth - width + offsetX;
		break;
	}
	case ALIGN_CENTER_X:
		[[fallthrough]];
	default:
	{
		*x = ((wWidth - width) / 2) + offsetX;
		break;
	}
	}

	switch (alignmentY)
	{
	case ALIGN_TOP:
	{
		*y = 0 + offsetY;
		break;
	}
	case ALIGN_BOTTOM:
	{
		*y = wHeight - height + offsetY;
		break;
	}
	case ALIGN_CENTER_Y:
		[[fallthrough]];
	default:
	{
		*y = ((wHeight - height) / 2) + offsetY;
		break;
	}
	}
}

void ui::image::init(window* win)
{
	renderer.setShader(Shader::get("tex"));
	renderer.setScaleOrigin({ 0.0f, 0.0f });
}

void ui::image::render(window* win)
{
	renderer.render();
}

void ui::image::windowResize(window* win, int width, int height)
{
	int x, y, w, h;
	getBounds(win, &x, &y, &w, &h);
	renderer.setPos(x, y);
}

void ui::image::setScale(float x, float y)
{
	renderer.setScale(x, y);
	updateSize();
}

void ui::image::updateSize()
{
	const Texture* texture = renderer.getTexture();
	if (texture != nullptr)
	{
		auto& size = texture->getSize();
		auto clip = renderer.getClip();
		auto scale = renderer.getScale();
		width = size.x * clip.z * scale.x;
		height = size.y * clip.w * scale.y;
	}
}

void ui::image::setTexture(const Texture* texture)
{
	renderer.setTexture(texture);
	updateSize();
}

void ui::image::setClip(float x, float y, float w, float h)
{
	renderer.setClip({ x, y, w, h });
	updateSize();
}

void ui::button::getBounds(window* win, int* x, int* y, int* w, int* h)
{
	*w = width;
	*h = height;

	int wWidth, wHeight;
	glfwGetFramebufferSize(win->getGlfwWindow(), &wWidth, &wHeight);

	switch (alignmentX)
	{
	case ALIGN_LEFT:
	{
		*x = 0 + offsetX;
		break;
	}
	case ALIGN_RIGHT:
	{
		*x = wWidth - width + offsetX;
		break;
	}
	case ALIGN_CENTER_X:
		[[fallthrough]];
	default:
	{
		*x = ((wWidth - width) / 2) + offsetX;
		break;
	}
	}

	switch (alignmentY)
	{
	case ALIGN_TOP:
	{
		*y = 0 + offsetY;
		break;
	}
	case ALIGN_BOTTOM:
	{
		*y = wHeight - height + offsetY;
		break;
	}
	case ALIGN_CENTER_Y:
		[[fallthrough]];
	default:
	{
		*y = ((wHeight - height) / 2) + offsetY;
		break;
	}
	}
}

void ui::button::render(window* win)
{
	int x, y, w, h;
	getBounds(win, &x, &y, &w, &h);
	qr.setPos(x, y, w, h);

	constexpr glm::vec3 colorA{ 20 / 255.0f, 160 / 255.0f, 46 / 255.0f };
	constexpr glm::vec3 colorB{ 121 / 255.0f, 58 / 255.0f, 128 / 255.0f };

	glm::vec3 textColor{ 1.0f, 1.0f, 1.0f };

	if (mouseDown)
	{
		qr.setColor(textColor);
		textColor = colorA;
	}
	else
	{
		double mousex, mousey;
		win->getCursorPos(&mousex, &mousey);
		if (mousex >= x && mousex < x + w && mousey >= y && mousey < y + h)
		{
			qr.setColor(colorA);
		}
		else
		{
			qr.setColor(colorB);
		}
	}

	qr.render();

	tr.setText(text);
	int tw = tr.getCharWidth() * 2 * text.size();
	int th = tr.getCharHeight() * 2;

	int tx = x + ((w - tw) / 2);
	int ty = y + ((h - th) / 2);

	tr.setFontSize(2);

	// text shadow
	tr.setPos(tx + 2, ty + 2);
	tr.setColor({ 0.0f, 0.0f, 0.0f });
	tr.render();

	// text
	tr.setPos(tx, ty);
	tr.setColor(textColor);
	tr.render();
}

bool ui::button::mouseButtonInput(window* win, double xpos, double ypos, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		mouseDown = true;
		return true;
	}
	else if (action == GLFW_RELEASE)
	{
		if (mouseDown)
		{
			if (this->action)
			{
				this->action();
			}
			mouseDown = false;
			return true;
		}
	}
	return false;
}

void ui::button::mouseDownCancel()
{
	mouseDown = false;
}

void ui::button::defocus()
{
	mouseDown = false;
}

void ui::button::setAction(std::move_only_function<void()> action)
{
	this->action = std::move(action);
}

void ui::button::setText(std::string_view text)
{
	this->text = text;
}

void ui::button::setSize(uint32_t w, uint32_t h)
{
	width = w;
	height = h;
}

TexRenderer ui::checkbox::checkRenderer;
void ui::checkbox::renderInit()
{
	Texture::load("assets/textures/check.png", 0, false, "check");
	checkRenderer.setTexture(Texture::get("check"));
	checkRenderer.setShader(Shader::get("tex"));
	checkRenderer.setScaleOrigin({ 0.0f, 0.0f });
	checkRenderer.setScale({ 2.0f, 2.0f });
}

void ui::checkbox::render(window* win)
{
	int x, y, w, h;
	getBounds(win, &x, &y, &w, &h);
	qr.setPos(x, y, BOX_SIZE, BOX_SIZE);

	constexpr glm::vec3 colorA{ 20 / 255.0f, 160 / 255.0f, 46 / 255.0f };
	constexpr glm::vec3 colorB{ 121 / 255.0f, 58 / 255.0f, 128 / 255.0f };

	glm::vec3 textColor{ 1.0f, 1.0f, 1.0f };

	if (mouseDown)
	{
		qr.setColor(textColor);
		textColor = colorA;
	}
	else
	{
		double mx, my;
		win->getCursorPos(&mx, &my);
		if (mx >= x && mx < x + w && my >= y && my < y + h)
		{
			qr.setColor(colorA);
		}
		else
		{
			qr.setColor(colorB);
		}
	}

	qr.render();

	if (checked)
	{
		checkRenderer.setPos(x, y);
		checkRenderer.render();
	}

	tr.setText(text);
	int tw = tr.getCharWidth() * 2 * text.size();
	int th = tr.getCharHeight() * 2;

	int tx = x + BOX_SIZE + 10;
	int ty = y + ((BOX_SIZE - th) / 2);

	tr.setFontSize(2);

	// text shadow
	tr.setPos(tx + 2, ty + 2);
	tr.setColor({ 0.0f, 0.0f, 0.0f });
	tr.render();

	// text
	tr.setPos(tx, ty);
	tr.setColor(textColor);
	tr.render();
}

void ui::checkbox::getBounds(window* win, int* x, int* y, int* w, int* h)
{
	int charWidth = tr.getCharWidth() * 2;
	int textWidth = text.length() * charWidth;

	*w = BOX_SIZE + textWidth + 10;
	*h = BOX_SIZE;

	int wWidth, wHeight;
	glfwGetFramebufferSize(win->getGlfwWindow(), &wWidth, &wHeight);

	switch (alignmentX)
	{
	case ALIGN_LEFT:
	{
		*x = 0 + offsetX;
		break;
	}
	case ALIGN_RIGHT:
	{
		*x = wWidth - *w + offsetX;
		break;
	}
	case ALIGN_CENTER_X:
		[[fallthrough]];
	default:
	{
		*x = ((wWidth - *w) / 2) + offsetX;
		break;
	}
	}

	switch (alignmentY)
	{
	case ALIGN_TOP:
	{
		*y = 0 + offsetY;
		break;
	}
	case ALIGN_BOTTOM:
	{
		*y = wHeight - BOX_SIZE + offsetY;
		break;
	}
	case ALIGN_CENTER_Y:
		[[fallthrough]];
	default:
	{
		*y = ((wHeight - BOX_SIZE) / 2) + offsetY;
		break;
	}
	}
}

bool ui::checkbox::mouseButtonInput(window* win, double xpos, double ypos, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		mouseDown = true;
		return true;
	}
	else if (action == GLFW_RELEASE)
	{
		if (mouseDown)
		{
			checked = !checked;
			if (this->action)
			{
				this->action(checked);
			}
			mouseDown = false;
			return true;
		}
	}
	return false;
}

void ui::checkbox::mouseDownCancel()
{
	mouseDown = false;
}

void ui::checkbox::defocus()
{
	mouseDown = false;
}

void ui::checkbox::setAction(std::move_only_function<void(bool checked)> action)
{
	this->action = std::move(action);
}

void ui::checkbox::setText(std::string_view text)
{
	this->text = text;
}

bool ui::checkbox::getChecked() const
{
	return checked;
}

void ui::checkbox::setChecked(bool checked)
{
	this->checked = checked;
}

void ui::text_input::render(window* win)
{
	int x, y, w, h;
	getBounds(win, &x, &y, &w, &h);

	qr.setPos(x, y, w, h);
	qr.setMode(QuadRenderer::MODE_FILL);
	qr.setColor({ 0, 0, 0, 0.5 });
	qr.render();

	// update the viewport
	win->changeViewport({ x, y, width, height }, { scrollPos, 0 });

	// render text
	tr.setFontSize(2);
	tr.align(false);

	int charWidth = tr.getCharWidth() * 2;
	int charHeight = tr.getCharHeight() * 2;
	// calculate center height for chars
	int centerHeight = (height - charHeight) / 2;

	// render cool effect for the last typed char
	if (float dt = glfwGetTime() - lastTypedCharTimer;
		dt < lastTypedCharTime)
	{
		float a = dt / lastTypedCharTime;

		std::string lastTypedChar;
		lastTypedChar.push_back(text[lastTypedCharIndex]);
		int charX = lastTypedCharIndex * charWidth;

		tr.setText(lastTypedChar);
		tr.setColor({ 0, 1, 0, a });
		tr.setPos(charX, centerHeight + (1.0f - a) * 10.0f);
		tr.render();
		tr.setColor({ 0.5f, 0, 1, a });
		tr.setPos(charX, centerHeight - (1.0f - a) * 10.0f);
		tr.render();
	}

	if (highlight && cursorPos != highlightStart)
	{
		int hStart = glm::min(cursorPos, highlightStart);
		int hEnd = glm::max(cursorPos, highlightStart);

		int hWidth = hEnd - hStart;

		// render pre-highlight portion
		if (hStart > 0)
		{
			std::string preH = text.substr(0, hStart);

			tr.setColor({ 1, 1, 1 });
			tr.setText(preH);
			tr.setPos(0, centerHeight);
			tr.render();
		}

		qr.setColor(0, 1, 0, 1);
		qr.setPos(hStart * charWidth, centerHeight - 2, hWidth * charWidth, charHeight + 2 + 2);
		qr.render();

		// render the highlighted text
		std::string hText = text.substr(hStart, hWidth);

		tr.setColor(0, 0, 0);
		tr.setText(hText);
		tr.setPos(hStart * charWidth, centerHeight);
		tr.render();

		// render the remaining characters
		if (hEnd < text.length())
		{
			std::string postH = text.substr(hEnd);

			tr.setColor({ 1, 1, 1 });
			tr.setText(postH);
			tr.setPos(hEnd * charWidth, centerHeight);
			tr.render();
		}
	}
	else
	{
		// render cursor
		bool showCursor = active && (int)((glfwGetTime() - cursorBlinkTimer) / cursorBlinkTime) % 2 == 0;
		if (showCursor)
		{
			int cursorX = cursorPos * charWidth - 2;
			qr.setColor(0.5f, 0, 1, 1);
			qr.setPos(cursorX, centerHeight + (tr.getCharHeight() * 2), tr.getCharWidth() * 2, 2);
			qr.render();
		}

		tr.setColor({ 1, 1, 1 });
		tr.setText(text);
		tr.setPos(0, centerHeight);
		tr.render();
	}
}

void ui::text_input::getBounds(window* win, int* x, int* y, int* w, int* h)
{
	*w = width;
	*h = height;

	int wWidth, wHeight;
	glfwGetFramebufferSize(win->getGlfwWindow(), &wWidth, &wHeight);

	switch (alignmentX)
	{
	case ALIGN_LEFT:
	{
		*x = 0 + offsetX;
		break;
	}
	case ALIGN_RIGHT:
	{
		*x = wWidth - width + offsetX;
		break;
	}
	case ALIGN_CENTER_X:
		[[fallthrough]];
	default:
	{
		*x = ((wWidth - width) / 2) + offsetX;
		break;
	}
	}

	switch (alignmentY)
	{
	case ALIGN_TOP:
	{
		*y = 0 + offsetY;
		break;
	}
	case ALIGN_BOTTOM:
	{
		*y = wHeight - height + offsetY;
		break;
	}
	case ALIGN_CENTER_Y:
		[[fallthrough]];
	default:
	{
		*y = ((wHeight - height) / 2) + offsetY;
		break;
	}
	}
}

bool ui::text_input::mouseInput(window* win, double xpos, double ypos)
{
	int x, y, w, h;
	getBounds(win, &x, &y, &w, &h);

	bool touchingMouse = xpos >= x && ypos >= y && xpos <= x + w && ypos <= y + h;

	if (mouseDown)
	{
		xpos -= x + scrollPos;
		ypos -= y;

		int charWidth = tr.getCharWidth() * 2;

		cursorPos = glm::max(0, glm::min((int)text.length(), (int)glm::round(xpos / charWidth)));
		highlight = true;

		updateScrollPos(win);
	}

	return touchingMouse;
}

bool ui::text_input::mouseButtonInput(window* win, double xpos, double ypos, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		active = touchingMouse(win);

		if (active)
		{
			mouseDown = true;

			int x, y, w, h;
			getBounds(win, &x, &y, &w, &h);

			double mx, my;
			win->getCursorPos(&mx, &my);
			mx -= x + scrollPos;
			my -= y;

			int charWidth = tr.getCharWidth() * 2;

			cursorPos = glm::max(0, glm::min((int)text.length(), (int)glm::round((float)mx / charWidth)));
			cursorBlinkTimer = glfwGetTime();

			highlight = false;
			highlightStart = cursorPos;
		}
	}
	else
	{
		mouseDown = false;

		if (cursorPos == highlightStart)
		{
			highlight = false;
		}
	}

	return active;
}

bool ui::text_input::keyInput(window* win, int key, int scancode, int action, int mods)
{
	if (action == GLFW_RELEASE || !enabled)
	{
		return false;
	}

	// these cases aren't included in the switch in order to handle the editable condition
	if (key == GLFW_KEY_LEFT)
	{
		if (mods & GLFW_MOD_SHIFT)
		{
			if (cursorPos > 0)
			{
				if (!highlight)
				{
					highlight = true;
					highlightStart = cursorPos;
				}
				--cursorPos;
				updateScrollPos(win);
			}
		}
		else if (highlight)
		{
			cursorPos = glm::min(cursorPos, highlightStart);
			highlightStart = cursorPos;
			highlight = false;
			updateScrollPos(win);
			cursorBlinkTimer = glfwGetTime();
		}
		else if (cursorPos > 0)
		{
			--cursorPos;
			updateScrollPos(win);
			cursorBlinkTimer = glfwGetTime();
		}
	}
	else if (key == GLFW_KEY_RIGHT)
	{
		if (mods & GLFW_MOD_SHIFT)
		{
			if (cursorPos < text.length())
			{
				if (!highlight)
				{
					highlight = true;
					highlightStart = cursorPos;
				}
				++cursorPos;
				updateScrollPos(win);
			}
		}
		else if (highlight)
		{
			cursorPos = glm::max(cursorPos, highlightStart);
			highlightStart = cursorPos;
			highlight = false;
			updateScrollPos(win);
			cursorBlinkTimer = glfwGetTime();
		}
		else if (cursorPos < text.length())
		{
			++cursorPos;
			updateScrollPos(win);
			cursorBlinkTimer = glfwGetTime();
		}
	}

	// don't allow any of the following actions if the input isn't editable
	if (!editable)
	{
		return false;
	}

	switch (key)
	{
	case GLFW_KEY_V:
	{
		if (mods & GLFW_MOD_CONTROL)
		{
			if (highlight)
			{
				removeHighlightedText();
				updateScrollPos(win);
			}

			std::string asciiVal = removeInvalidChars(glfwGetClipboardString(win->getGlfwWindow()));
			text.insert(cursorPos, asciiVal);
			cursorPos += asciiVal.length();
			updateScrollPos(win);
		}
		break;
	}
	case GLFW_KEY_C:
	{
		if (mods & GLFW_MOD_CONTROL)
		{
			if (highlight)
			{
				glfwSetClipboardString(win->getGlfwWindow(), getTextHighlighted().c_str());
			}
		}
		break;
	}
	case GLFW_KEY_X:
	{
		if (mods & GLFW_MOD_CONTROL)
		{
			if (highlight)
			{
				glfwSetClipboardString(win->getGlfwWindow(), getTextHighlighted().c_str());
				removeHighlightedText();
				updateScrollPos(win);
			}
		}
		break;
	}
	case GLFW_KEY_DELETE:
		if (highlight)
		{
			int startLength = text.length();
			removeHighlightedText();
			lastTypedCharTimer = 0;
			updateScrollBackspace(win, startLength - text.length());
		}
		else if (cursorPos < text.length())
		{
			text.erase(text.begin() + cursorPos);
			lastTypedCharTimer = 0;
			updateScrollBackspace(win, 1);
		}
		cursorBlinkTimer = glfwGetTime();
		break;
	case GLFW_KEY_BACKSPACE:
		if (highlight)
		{
			int startLength = text.length();
			removeHighlightedText();
			lastTypedCharTimer = 0;
			updateScrollBackspace(win, startLength - text.length());
		}
		else if (cursorPos > 0)
		{
			text.erase(text.begin() + cursorPos - 1);
			--cursorPos;
			lastTypedCharTimer = 0;
			updateScrollBackspace(win, 1);
		}
		cursorBlinkTimer = glfwGetTime();
		break;
	case GLFW_KEY_A:
		if ((mods & GLFW_MOD_CONTROL) && !text.empty())
		{
			highlight = true;
			highlightStart = 0;
			cursorPos = text.size();
			updateScrollPos(win);
		}
		break;
	default:
		return false;
		break;
	}

	return true;
}

bool ui::text_input::charInput(window* win, uint32_t codepoint)
{
	if (!enabled || !editable)
	{
		return false;
	}

	if (codepoint < 128)
	{
		unsigned char c = codepoint & 0xFF;

		if (c >= 32 && c < 127)
		{
			if (highlight)
			{
				removeHighlightedText();
				updateScrollPos(win);
			}

			// ascii character
			text.insert(text.begin() + cursorPos, c);

			cursorBlinkTimer = glfwGetTime();
			lastTypedCharTimer = glfwGetTime();
			lastTypedCharIndex = cursorPos;

			++cursorPos;

			updateScrollPos(win);
		}
	}

	return true;
}

void ui::text_input::mouseDownCancel()
{
	mouseDown = false;
}

void ui::text_input::focus()
{
	active = true;
}

void ui::text_input::defocus()
{
	active = false;
	highlight = false;
	highlightStart = cursorPos;
}

void ui::text_input::setText(std::string_view text)
{
	this->text = text;
}

std::string ui::text_input::getText() const
{
	return text;
}

std::string ui::text_input::getTextHighlighted() const
{
	int hStart = glm::min(cursorPos, highlightStart);
	int hEnd = glm::max(cursorPos, highlightStart);
	return text.substr(hStart, hEnd - hStart);
}

void ui::text_input::setSize(uint32_t w, uint32_t h)
{
	width = w;
	height = h;
}

void ui::text_input::updateScrollPos(window* win)
{
	int charWidth = tr.getCharWidth() * 2;

	int cursorOffset = maxScrollPos + cursorPos * charWidth;

	if (cursorOffset + scrollPos >= width)
	{
		scrollPos = -cursorOffset + width - charWidth;
	}
	else if (cursorOffset + scrollPos - charWidth * 2 <= 0)
	{
		scrollPos = -cursorOffset + charWidth * 2;
	}

	// final adjustment
	scrollPos = glm::min(maxScrollPos, scrollPos);
}

void ui::text_input::updateScrollBackspace(window* win, int deletedCharCount)
{
	int charWidth = tr.getCharWidth() * 2;
	// final adjustment
	scrollPos = glm::min(maxScrollPos, scrollPos + deletedCharCount * charWidth);
}

bool ui::text_input::touchingMouse(window* win)
{
	double mx, my;
	win->getCursorPos(&mx, &my);

	int x, y, w, h;
	getBounds(win, &x, &y, &w, &h);

	return (mx >= x && mx < x + w && my >= y && my < y + h);
}

std::string ui::text_input::removeInvalidChars(const char* str)
{
	std::string result;

	if (str == nullptr)
	{
		return result;
	}

	int i = 0;
	while (str[i] != '\0')
	{
		unsigned char c = str[i];
		unsigned char mask = 128;

		if ((c & mask) == 0)
		{
			++i;

			// check for control chars
			if (c >= 32 && c < 127)
			{
				result.insert(result.end(), c);
			}

			continue;
		}

		// use the flag bits to determine how many chars to skip
		do {
			++i;
			mask >>= 1;
		} while (c & mask);

	}

	return result;
}

void ui::text_input::removeHighlightedText()
{
	int hStart = glm::min(cursorPos, highlightStart);
	int hEnd = glm::max(cursorPos, highlightStart);
	text.erase(text.begin() + hStart, text.begin() + hEnd);
	highlight = false;
	cursorPos = hStart;
	highlightStart = cursorPos;
}