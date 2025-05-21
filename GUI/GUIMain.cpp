#include "GUIMain.h"
#include "Core.h"
#include "ProblemHandler.h"
#include "gwindow.h"
#include "gevent.h"
#include "gbutton.h"
#include "gthread.h"
#include "map.h"
#include <chrono>
#include <cstdlib>
    using namespace std;

    namespace {
    // === Constants for window size and timing ===
    const double kWindowWidth  = 1000;
    const double kWindowHeight = 800;

    // Timer events older than this (ms) are ignored to avoid lag loops
    const long kTimelyCutoff = 100;

    // Function type alias for problem constructors
    using Constructor = std::function<void()>;

    // === Main graphics state struct ===
    struct Graphics {
        GWindow window{kWindowWidth, kWindowHeight};              // The main window
        shared_ptr<ProblemHandler> handler;                      // Current problem handler
        Map<GObservable*, Constructor> constructors;             // Button to handler mapping
    };

    // === Initialize the graphics window and buttons ===
    Graphics* makeGraphics() {
        auto* result = new Graphics();

        result->window.setTitle(MiniGUI::Config::programTitle());
        result->window.setCloseOperation(GWindow::CLOSE_DO_NOTHING);
        result->window.setRepaintImmediately(false);
        result->window.setCanvasSize(kWindowWidth, kWindowHeight);

        // Add buttons for each menu option (Sierpinski, Recursia, etc.)
        for (const auto& entry: MiniGUI::Config::menuOptions()) {
            auto* button = new GButton(entry.name);
            result->window.addToRegion(button, "NORTH");
            result->constructors[button] = entry.callback;
        }

        return result;
    }

    // === Switch to a new problem if possible ===
    void setProblem(Graphics* graphics, GObservable* source) {
        // Don't switch if the current handler refuses to shut down
        if (graphics->handler && !graphics->handler->shuttingDown()) return;

        auto constructor = graphics->constructors.get(source);
        if (!constructor) error("No constructor for that GObservable?");

        // Reset UI in the GUI thread to avoid flicker
        GThread::runOnQtGuiThread([&] {
            graphics->handler.reset();               // Remove old handler
            graphics->window.clearCanvas();          // Clear display
            constructor();                           // Call constructor for new handler
        });

        graphics->handler->settingUp();
    }

    // Global state
    Graphics* theGraphics = nullptr;
    bool theOptionsEnabled = true;
    }

    // === Enables or disables problem buttons ===
    void setDemoOptionsEnabled(bool isEnabled) {
        for (GObservable* option: theGraphics->constructors) {
            if (auto* button = dynamic_cast<GButton*>(option)) {
                button->setEnabled(isEnabled);
            }
        }
        theOptionsEnabled = isEnabled;
    }

    namespace MiniGUI {
    namespace Detail {

    // Return a reference to the graphics window
    GWindow& graphicsWindow() {
        if (!theGraphics) error("Graphics window not available.");
        return theGraphics->window;
    }

    // Swap in a new active ProblemHandler (from inside a constructor)
    void setActiveDemo(shared_ptr<ProblemHandler> handler) {
        theGraphics->handler = handler;
    }

    // === The actual event-driven main loop ===
    void graphicsMain(function<void()> initialDemo) {
        theGraphics = makeGraphics();

        // Call the initial demo's constructor on the GUI thread
        GThread::runOnQtGuiThread([&] {
            initialDemo();
        });
        theGraphics->handler->settingUp();

        while (true) {
            // Draw handler's canvas content (if needed)
            theGraphics->handler->draw();

            // Wait for any kind of GUI event
            GEvent e = waitForEvent(MOUSE_EVENT | ACTION_EVENT | CHANGE_EVENT |
                                    TIMER_EVENT | WINDOW_EVENT | HYPERLINK_EVENT);

            if (e.getEventClass() == ACTION_EVENT) {
                auto source = GActionEvent(e).getSource();
                if (theGraphics->constructors.containsKey(source)) {
                    if (theOptionsEnabled) setProblem(theGraphics, source);
                } else {
                    theGraphics->handler->actionPerformed(source);
                }

            } else if (e.getEventClass() == CHANGE_EVENT) {
                theGraphics->handler->changeOccurredIn(GChangeEvent(e).getSource());

            } else if (e.getEventClass() == TIMER_EVENT) {
                long now = chrono::duration_cast<chrono::milliseconds>(
                               chrono::system_clock::now().time_since_epoch()).count();
                if (now - e.getTime() < kTimelyCutoff) {
                    theGraphics->handler->timerFired();
                }

            } else if (e.getEventClass() == MOUSE_EVENT) {
                if (e.getSource() == theGraphics->window.getCanvas()) {
                    if (e.getEventType() == MOUSE_MOVED) {
                        theGraphics->handler->mouseMoved(e.getX(), e.getY());
                    } else if (e.getEventType() == MOUSE_PRESSED) {
                        theGraphics->handler->mousePressed(e.getX(), e.getY());
                    } else if (e.getEventType() == MOUSE_DRAGGED) {
                        theGraphics->handler->mouseDragged(e.getX(), e.getY());
                    } else if (e.getEventType() == MOUSE_RELEASED) {
                        theGraphics->handler->mouseReleased(e.getX(), e.getY());
                    } else if (e.getEventType() == MOUSE_EXITED) {
                        theGraphics->handler->mouseExited();
                    } else if (e.getEventType() == MOUSE_CLICKED) {
                        theGraphics->handler->mouseClicked(e.getX(), e.getY());
                    } else if (e.getEventType() == MOUSE_DOUBLE_CLICKED) {
                        theGraphics->handler->mouseDoubleClicked(e.getX(), e.getY());
                    }
                }

            } else if (e.getEventClass() == WINDOW_EVENT) {
                if (e.getSource() == &theGraphics->window) {
                    if (e.getEventType() == WINDOW_MAXIMIZED ||
                        e.getEventType() == WINDOW_RESIZED   ||
                        e.getEventType() == WINDOW_RESTORED) {
                        theGraphics->handler->windowResized();
                    } else if (e.getEventType() == WINDOW_CLOSING &&
                               theGraphics->handler->shuttingDown()) {
                        theGraphics->handler.reset();
                        break;
                    }
                }

            } else if (e.getEventClass() == HYPERLINK_EVENT) {
                theGraphics->handler->hyperlinkClicked(e.getRequestURL());
            }
        }

        // === Final shutdown ===
        // TODO: Gracefully handle shutdown if Stanford C++ lib ever fixes GUI destructors
        _Exit(0); // Immediate exit to avoid crashing destructors
    }
    }
    }
