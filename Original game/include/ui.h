#pragma once
#include "QuadRenderer.h"
#include "TextRenderer.h"
#include "TexRenderer.h"
#include <cstdint>
#include <vector>
#include <string>

struct GLFWwindow;

namespace ui
{
	enum alignx
	{
		ALIGN_LEFT,
		ALIGN_RIGHT,
		ALIGN_CENTER_X,
	};

	enum aligny
	{
		ALIGN_TOP,
		ALIGN_BOTTOM,
		ALIGN_CENTER_Y,
	};
	
	class window
	{
	public:
		virtual GLFWwindow* getGlfwWindow() = 0;
		virtual void getCursorPos(double* xpos, double* ypos) = 0;
		virtual void changeViewport(const glm::ivec4& pos, const glm::ivec2& scroll) = 0;
	};

	class element;
	class page : public window
	{
	public:

		// constants & types
		// -----------------

		using container_t = std::vector<element*>;

		// constructors
		// ------------

		page();

		// window method overrides
		// -----------------------

		GLFWwindow* getGlfwWindow() override;
		void getCursorPos(double* xpos, double* ypos) override;
		void changeViewport(const glm::ivec4& pos, const glm::ivec2& scroll) override;

		// class-specific methods
		// ----------------------

		void addElem(element* e);
		bool removeElem(element* e);
		void removeElem(container_t::iterator it);
		void clear();

		container_t::iterator begin();
		container_t::iterator end();

		container_t::const_iterator begin() const;
		container_t::const_iterator end() const;
		
		void init(GLFWwindow* w);
		void render();
		void mouseInput(double xpos, double ypos);
		bool scrollInput(double xoff, double yoff);
		bool mouseButtonInput(int button, int action, int mods);
		bool keyInput(int key, int scancode, int action, int mods);
		bool charInput(uint32_t codepoint);
		bool fileDrop(int count, const char* paths[]);

		void windowResize(int width, int height);
		// returns nullptr if there is no focused element
		element* getFocusedElem();
		element* getMouseDownElem();

	private:

		container_t elems;
		GLFWwindow* window = nullptr;

		int focusIndex = -1;
		int mouseDownIndex = -1;

		void mouseDownCancel();

		bool viewportUpdated = false;

		int currentCursorType = GLFW_ARROW_CURSOR;
		GLFWcursor* currentCursor = nullptr;
	};
	
	class element
	{
	public:
		
		// static methods
		// --------------

		static void renderInit();

		// virtual methods
		// ---------------

		virtual void init(window* win) {}
		virtual void render(window* win) {}

		virtual bool clickable() { return false; }
		virtual void getBounds(window* win, int* x, int* y, int* w, int* h) = 0;

		virtual bool mouseInput(window* win, double xpos, double ypos) { return false; }
		virtual bool scrollInput(window* win, double xoff, double yoff) { return false; }
		virtual bool mouseButtonInput(window* win, double xpos, double ypos, int button, int action, int mods) { return false; }
		virtual bool keyInput(window* win, int key, int scancode, int action, int mods) { return false; }
		virtual bool charInput(window* win, uint32_t codepoint) { return false; }
		virtual bool fileDrop(window* win, int count, const char* paths[]) { return false; }

		// called when the left mouse button was pressed over the element but is released off of it
		virtual void mouseDownCancel() {}

		virtual void windowResize(window* win, int width, int height) {}

		virtual void focus() {}
		virtual void defocus() {}
		
		virtual int getCursorType() { return GLFW_ARROW_CURSOR; }

		// non-virtual methods
		// -------------------

		alignx getAlignX() const;
		aligny getAlignY() const;

		void setAlignX(alignx alignment);
		void setAlignY(aligny alignment);

		int getOffsetX() const;
		int getOffsetY() const;

		void setOffsetX(int offset);
		void setOffsetY(int offset);

	protected:

		static QuadRenderer qr;
		static TextRenderer tr;

		alignx alignmentX;
		aligny alignmentY;

		int offsetX;
		int offsetY;
	};

	class text : public element
	{
	public:

		// element overrides
		// -----------------

		//void init(window* win) override;
		void render(window* win) override;

		void getBounds(window* win, int* x, int* y, int* w, int* h) override;

		// class-specific methods
		// ----------------------

		void setText(std::string_view value);

		int getSize() const;
		void setSize(uint32_t size);

		// set to zero for no wrap width
		void setWrapWidth(uint32_t wrapWidth);

		void setColor(const glm::vec4& color);

		void setShadow(bool shadow);

	private:

		std::string text;
		uint32_t size = 2;
		uint32_t wrapWidth = 0;
		glm::vec4 color{ 1.0f };
		bool shadow = true;

		void getBoundSize(int* w, int* h);

		void renderText(window* win, std::string_view text, int x, int y, bool align);
	};

	class image : public element
	{
	public:

		// element overrides
		// -----------------

		void init(window* win) override;
		void render(window* win) override;

		void getBounds(window* win, int* x, int* y, int* w, int* h) override;

		void windowResize(window* win, int width, int height) override;

		// class-specific methods
		// ----------------------

		void setTexture(const Texture* texture);
		void setClip(float x, float y, float w, float h);
		void setScale(float x, float y);

	private:

		const Texture* texture = nullptr;
		TexRenderer renderer;

		int width;
		int height;

		void updateSize();
	};

	class button : public element
	{
	public:

		// element overrides
		// -----------------

		void render(window* win) override;

		bool clickable() override { return true; }
		void getBounds(window* win, int* x, int* y, int* w, int* h) override;

		bool mouseButtonInput(window* win, double xpos, double ypos, int button, int action, int mods) override;

		void mouseDownCancel() override;

		void defocus() override;

		// class-specific methods
		// ----------------------

		void setAction(std::move_only_function<void()> action);
		void setText(std::string_view value);
		void setSize(uint32_t w, uint32_t h);

	private:

		std::string text;

		uint32_t width = 200;
		uint32_t height = 50;

		bool mouseDown = false;

		std::move_only_function<void()> action;
	};

	class checkbox : public element
	{
	public:

		// static methods
		// --------------

		static void renderInit();

		// element overrides
		// -----------------

		void render(window* win) override;

		bool clickable() override { return true; }
		void getBounds(window* win, int* x, int* y, int* w, int* h) override;

		bool mouseButtonInput(window* win, double xpos, double ypos, int button, int action, int mods) override;

		void mouseDownCancel() override;

		void defocus() override;

		// class-specific methods
		// ----------------------

		void setAction(std::move_only_function<void(bool checked)> action);
		void setText(std::string_view value);

		bool getChecked() const;
		void setChecked(bool checked);

	private:

		static TexRenderer checkRenderer;

		static constexpr uint32_t BOX_SIZE = 24;

		std::string text;

		bool mouseDown = false;

		std::move_only_function<void(bool checked)> action;

		bool checked = false;
	};

	class text_input : public element
	{
	public:

		// element overrides
		// -----------------

		//void init(window* win) override;
		void render(window* win) override;

		bool clickable() override { return true; }
		void getBounds(window* win, int* x, int* y, int* w, int* h) override;

		bool mouseInput(window* win, double xpos, double ypos) override;
		bool mouseButtonInput(window* win, double xpos, double ypos, int button, int action, int mods) override;
		bool keyInput(window* win, int key, int scancode, int action, int mods) override;
		bool charInput(window* win, uint32_t codepoint) override;

		void mouseDownCancel() override;

		void focus() override;
		void defocus() override;

		int getCursorType() override { return GLFW_IBEAM_CURSOR; }

		// class-specific methods
		// ----------------------

		void setText(std::string_view text);
		std::string getText() const;
		std::string getTextHighlighted() const;

		void setSize(uint32_t w, uint32_t h);

	private:

		std::string text;
		bool enabled = true;
		bool editable = true;
		bool mouseDown = false;
		// is the input capturing key presses?
		bool active = false;
		bool highlight = false;

		int cursorPos = 0;
		int highlightStart = 0;
		uint32_t width = 100;
		uint32_t height = 50;

		static constexpr int maxScrollPos = 4;
		int scrollPos = maxScrollPos;
		void updateScrollPos(window* win);
		void updateScrollBackspace(window* win, int deletedCharCount);

		// cursor blink time in seconds
		static constexpr float cursorBlinkTime = 0.5f;
		float cursorBlinkTimer = 0;

		static constexpr float lastTypedCharTime = 0.2f;
		float lastTypedCharTimer = 0;
		int lastTypedCharIndex = 0;

		int xOffset = 0;
		int yOffset = 0;
		alignx alignmentx = ALIGN_LEFT;
		aligny alignmenty = ALIGN_TOP;

		bool touchingMouse(window* win);

		// removes the UTF-8 encoded chars from a string.
		// also removes control chars like newlines
		std::string removeInvalidChars(const char* str);

		void removeHighlightedText();
	};
}