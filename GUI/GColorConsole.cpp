#include "GColorConsole.h"
#include "gthread.h"
#include "strlib.h"
using namespace std;

namespace {
// Basic HTML structure used to display the colored console output in a browser-like GUI
const string kHTMLHeader = R"(
         <html>
            <head></head>
            <body style="background-color:white;color:black;">
                <pre>)";

const string kHTMLFooter = R"(</pre>
            </body>
        </html>
    )";
}

// Constructor sets up our stream to use a custom buffer.
GColorConsole::GColorConsole() : ostream(new ConsoleStreambuf(this)) {
    // All setup is done in initializer list
}

// Destructor safely deletes the stream buffer to avoid memory leaks
GColorConsole::~GColorConsole() {
    delete rdbuf(); // this was allocated with new in the initializer list
}

// Clears all displayed contents, but we wait to update the GUI until updateDisplay is called.
void GColorConsole::clearDisplay() {
    flushBuffer();    // flush any current content to mContents
    mContents.clear(); // then clear the stored lines
}

// This function is triggered when stream needs to be synchronized (like flushing with endl)
int GColorConsole::ConsoleStreambuf::sync() {
    mOwner->updateDisplay(); // tells the GUI to redraw everything
    return 0;
}

// Flush any buffered stream output and append it to our stored output lines
void GColorConsole::flushBuffer() {
    auto* buffer = static_cast<ConsoleStreambuf *>(rdbuf());

    auto contents = buffer->str(); // get whatever was being written
    if (contents.empty()) return;  // nothing to flush

    buffer->str(""); // reset the buffer
    mContents.emplace_back(mStyle, contents); // store the styled string
}

// This function rebuilds the entire display as HTML and updates the GUI console
void GColorConsole::updateDisplay() {
    flushBuffer(); // Make sure all new text is added before rendering

    stringstream toShow;
    toShow << kHTMLHeader;

    for (const auto& line: mContents) {
        // Open a span with style
        toShow << "<span style=\"";
        toShow << "color:" << line.first.color.toHTML() << ";";
        if (line.first.fontStyle & BOLD)   toShow << "font-weight:bold;";
        if (line.first.fontStyle & ITALIC) toShow << "font-style:italic;";
        toShow << "font-size:" << line.first.fontSize.size() << "pt;";
        toShow << "\">";

        // Escape HTML-sensitive characters
        toShow << htmlEncode(line.second);

        toShow << "</span>"; // close style span
    }

    toShow << kHTMLFooter; // finish HTML structure

    // We need to do GUI updates on the main thread
    GThread::runOnQtGuiThread([&, this] {
        setText(toShow.str()); // replace the text content
        scrollToBottom();      // scroll to the newest output
    });
}

// Set the current style (color, font style, size) for the output
void GColorConsole::setStyle(MiniGUI::Color color, FontStyle style, FontSize size) {
    flushBuffer(); // flush any text that was using the previous style
    mStyle.color = color;
    mStyle.fontStyle = style;
    mStyle.fontSize = size;
}

// Simple getter functions
GColorConsole::FontStyle GColorConsole::style() const {
    return mStyle.fontStyle;
}
MiniGUI::Color GColorConsole::color() const {
    return mStyle.color;
}
FontSize GColorConsole::fontSize() const {
    return mStyle.fontSize;
}

// Temporarily change style for the duration of a function, then restore original style
void GColorConsole::doWithStyle(MiniGUI::Color newColor, FontStyle newStyle, FontSize newSize, std::function<void ()> fn) {
    // Save the current style
    auto oldColor = color();
    auto oldStyle = style();
    auto oldSize  = fontSize();

    setStyle(newColor, newStyle, newSize); // apply new style

    try {
        fn(); // call the user function using the new style
    } catch (...) {
        setStyle(oldColor, oldStyle); // restore if it fails
        throw;
    }

    setStyle(oldColor, oldStyle, oldSize); // restore after function completes
}

// Overloads that allow calling doWithStyle with fewer arguments
void GColorConsole::doWithStyle(MiniGUI::Color color, FontStyle style, std::function<void ()> fn) {
    doWithStyle(color, style, fontSize(), fn);
}
void GColorConsole::doWithStyle(MiniGUI::Color color, std::function<void ()> fn) {
    doWithStyle(color, style(), fontSize(), fn);
}
void GColorConsole::doWithStyle(FontStyle style, std::function<void ()> fn) {
    doWithStyle(color(), style, fontSize(), fn);
}
void GColorConsole::doWithStyle(FontStyle style, FontSize size, std::function<void()> fn) {
    doWithStyle(color(), style, size, fn);
}
void GColorConsole::doWithStyle(MiniGUI::Color color, FontSize size, std::function<void()> fn) {
    doWithStyle(color, style(), size, fn);
}
void GColorConsole::doWithStyle(FontSize size, std::function<void()> fn) {
    doWithStyle(color(), style(), size, fn);
}

// Simple wrapper class for font size
FontSize::FontSize(size_t size): mSize(size) {
    // constructor body not needed
}

size_t FontSize::size() const {
    return mSize;
}
