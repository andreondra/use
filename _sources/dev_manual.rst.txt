Developer's Guide
##################

.. toctree::
   :maxdepth: 2
   :caption: Contents:

This guide describes the structure and inner workings of the platform. After reading this guide, you
will be able to create your own components and systems.

.. tip::
   Class names are also links to their respective documentation; just click 🔘👈 them to be redirected. Or you can check
   out the :ref:`API docs` yourself. The code documentation is always auto-generated from the latest commit to the main branch.

.. note::
   If you understand Czech, you can read a `bachelor's thesis <https://github.com/andreondra/bachelor-thesis>`_
   for which the project was developed and describes not only the project, but also a matter of emulation
   and the NES console in a great depth.

USE as a platform consists of multiple classes.

Internal classes
=================
The main class represeting whole application is :class:`Emulator`.
It takes care of configuring the rendering backend and other app configurations, renders global window,
loads a chosen :class:`System`, configures sound output (class :class:`Sound`), runs the emulation and
passes sound output from :class:`System` to :class:`Sound`. Class :class:`Sound` uses miniaudio library
and handles all of the sound processing, including buffering and low-pass filtering -- you do not have to worry about anything.
There are also some tools included in the Tools.cpp file. These were the platform's "internal" classes/files.

Component
==========
The most important class, where implementation of all the emulated components takes place, is the :class:`Component`.
This abstract class is a common interface for all of the components -- everything is a component: bus, memory, CPU...
Before anything else, you usually want to implement a component, see :ref:`Creating your own component`.

Every component usually needs to communicate with other components, so you will need to define an interface. Luckily for you,
universal communication interface for every component is already present in the platform. The interface consists of
two parts: :class:`Port` and :class:`Connector`. See :ref:`Port and Connector` to understand how they work.

Creating your own component
****************************
To create your own component, you need to inherit from the :class:`Component` class. Do not forget to
separate class declaration (belongs to header file) and definitions (belongs to cpp file). Then you need to implement
these three methods:

* :func:`Component::init()`: this method is usually called on startup or reset of a system. Define a proper default power-on state of your component here.
* :func:`Component::getGUIs()`: in this method you implement a GUI for your component using Dear ImGui library. Do not worry, it is very easy.
* A constructor of your class. There you define ports and connectors, see :ref:`Port and Connector` and set a name your component: ``m_deviceName = "myName"``

Optionally, you may override other methods, see :class:`Component` to find out which are available. You may be also
interested in :ref:`Keyboard input handling` and :ref:`Playing sounds`.

You can use this template for your component's class declaration. Do not hesitate to check other components for inspiration.

.. code-block:: cpp

    class myComponent : public Component{

    private:
        // There you put your ports and internal data.

    public:
        myComponent(/*here you can put some parameters*/);

        // Power-on state of your component is implemented here (resetting registers, memories...).
        void init() override;
        // GUI rendering is defined here.
        std::vector<EmulatorWindow> getGUIs() override;
    };

And here's a template for :func:`Component::getGUIs()` method:

.. code-block:: cpp

    std::vector<EmulatorWindow> myComponent::getGUIs() {

        std::function<void(void)> myWindow1 = [this](){

            // Window contents
            // ===================================================================
            ImGui::SeparatorText("Registers");
            ImGui::Text("Register 1: 0x%x", m_reg1);
            ImGui::Text("Register 2: 0x%x", m_reg2);
            ImGui::Separator();
            ImGui::Checkbox("Allow writing", &m_writingAllowed);
        };

        return {
                EmulatorWindow{
                        // This will be shown as a prefix in square brackets: [myComponentName].
                        // Usually it is a name of your component.
                        .category = m_deviceName,
                        // Title which follows after the prefix.
                        .title = "myWindow1",
                        // Unique ID, you can leave this default.
                        .id    = getDeviceID(),
                        // Where do you want your window to be docked. See Types.h for available docks.
                        .dock  = DockSpace::LEFT,
                        // Pass the rendering lambda you defined above.
                        .guiFunction = myWindow1
                },
                /*
                optionally another EmulatorWindow {...} and another and another...
                */
        };
    }

.. tip::
    You can define as many lambdas as you wish, do not forget to add them to the return statement.
    More sophisticated examples are in the components, check for example Memory.cpp to see how to show
    modals and file pickers, or just check out Dear ImGui to see which widgets are available.

Keyboard input handling
*************************
If your component wants to interact with any keys or gamepad buttons, you need to also override :func:`Component::getInputs()` method.
In this function, you return a vector of "actions". For example, if your components represents a gamepad
that contains two buttons (left and right), you define two actions:

.. code-block:: cpp

    std::vector<ImInputBinder::action_t> myComponent::getInputs() {
        return {
            ImInputBinder::action_t {
                .name_id = "Left Button",
                .key     = ImGuiKey_LeftArrow,
                .pressCallback = [&](){ m_controller.setLeft(true); },
                .releaseCallback = [&](){ m_controller.setLeft(false); },
            },
            ImInputBinder::action_t {
                .name_id = "Right Button",
                .key     = ImGuiKey_RightArrow,
                .pressCallback = [&](){ m_controller.setRight(true); },
                .releaseCallback = [&](){ m_controller.setRight(false); },
            },
        }
    }

.. note::
    Assigned buttons can be changed later by the user, which is saved automatically to a configuration file.
    The platform uses `ImInputBinder <https://github.com/andreondra/ImInputBinder>`_ library to handle user inputs,
    check it out to find more. Also, keys and buttons are identified by the Dear ImGui's names, check the
    ImGui's documentation to find a list of the available keys.

Playing sounds
****************
If your component wants to play some sounds, you need to override :func:`Component::getSoundSampleSources()`.
This functions returns a vector of functions which return normalized float amplitude value [-1,1] (miniaudio's ``ma_format_f32``)
for two channels (left and right). If your component has only a mono output, return the same value for both channels.
Here is an example of the method of a component which has only a single sound source:

.. code-block:: cpp

    SoundSampleSources myComponent::getSoundSampleSources() {
        return {
          [&](){
              SoundStereoFrame frame {leftOutput(), rightOutput()};
              return frame;
          }
        };
    }

.. tip::
    There can be as many sound sources as you wish, they will be mixed automatically. Note that you
    only need to return two floats, that's all. All synchronization, buffering and so on will be done automatically as well.
    Just don't forget to define correct main clock value.

Port and Connector
===================
When two components communicate, usually there is a component which controls the communication (active) and the other component
which is communicated with (passive). For example, when a CPU wants to write something to the memory, the CPU is the active one
and the memory passive. However, if there is for example a button connected directly to the CPU (for example to trigger an interrupt), the button is now the active
and the CPU is the passive component. To sum up, in a communication, there is always one component (active) which sends something to the other one (passive).

Port
*****
If your component wants to control any other component, you have to provide a :class:`Port`. In the code you then
interface with the port, which represents whatever component is connected to the port. To provide a port,
you usually place a selected type of the port as a data member to your class:

.. code-block:: cpp

    class myComponent : public Component {
    // ...
    private:
        DataPort m_mainBus;
        SignalPort m_INT;
    // ...
    }

Then to make *magic happen* you have to place your ports to the internal port map in the constructor:

.. code-block:: cpp

    myComponent::myComponent() {
        m_ports["mainBus"] = &m_mainBus;
        m_ports["INT"]     = &m_INT;
    }

That it is. Now your component has a platform-compatible interface defined. To use the interface in your code,
you can use functions which offers your selected port type.

:class:`DataPort` offers methods for reading and writing the data, :class:`SignalPort` offers two methods: :func:`SignalPort::set`
to set a logical voltage value (high, low) of a pin of a connected device, :func:`SignalPort::send` is used to send a simple pulse (where a specific value
is not important, e.g. clock pulses). Check classes :class:`DataPort` and :class:`SignalPort` for signatures of available methods.

Connector
***********
If you want to be your component controlled by any other, you have to specify a :class:`Connector`. Connectors
are usually declared and defined directly in the constructor of your component. There are two types of
interfaces for your connector: :class:`DataInterface` and :class:`SignalInterface`. Depending on chosen interface
the connector will be compatible with different port types:

.. list-table:: Connector interface and Port compabitility
   :widths: 25 25
   :header-rows: 1

   * - Connector interface
     - Port
   * - :class:`DataInterface`
     - :class:`DataPort`
   * - :class:`SignalInterface`
     - :class:`SignalPort`

Here are some examples. First a demonstration of :class:`SignalInterface`.

.. code-block:: cpp

   myComponent::myComponent() {

        m_connectors["myConnector1"] = std::make_shared<Connector>(SignalInterface{
            .send = [this]() {
                // Do something when someone sends a pulse.
            },
            .set = [this](bool active) {
                if (active) {
                    // Do something when someone sets a pin value to 1 (high).
                } else {
                    // Do something when someone sets a pin value to 0 (low).
                }
            }
        });
   }

As you can see, :class:`SignalInterface` offers two methods. Method :member:`SignalInterface::set` is meant to be used to
set a logic level of a certain pin. Method :member:`SignalInterface::send` is meant to send only a pulse of a "truthy" logic value where you
only need to trigger an action and specific value is not important (e.g. clock pins). You can choose to implement
only one method and leave the second unimplemented.

Now an example of :class:`DataInterface`:

.. code-block:: cpp

    // This example component has a single data interface represented by "myConnector".
    // It contains register at 0x0020 available for reading and writing.
    // Register at 0x0021 is write-only.
    m_connectors["myConnector"] = std::make_shared<Connector>( DataInterface {

        .read = [&](uint32_t address, uint32_t & buffer) -> bool {
            if (address == 0x0020) {
                buffer = m_register20;
                return true;
            } else {
                return false;
            }
        },
        .write = [&](uint32_t address, uint32_t data) {

            if (address == 0x0020) {
                m_register20 = data;
            } else if (address == 0x0021) {
                m_register21 = data;
            }
        }
    });

:class:`DataInterface` offers also two methods. First method :func:`DataInterface::read` is used
by other component to read something at specified address from your component. You either write value you want to respond with
to the ``buffer`` parameter and return ``true`` or do not write anything and return false (when your component do not
respond to that particular address). Second method :func:`DataInterface::write` is used to write something
to your component. If your component is mapped to the address specified, you can take the value in `data` and do whatever with it,
if your component is not mapped, do not do anything.

.. warning::
    Please be aware that DataInterface's method can be called even when the communication is not meant for your component.
    For example if your component is connected to a :class:`Bus`, everything is written to every component just like in a real
    bus and it is up to the component whether to care about the written data. It is the same with reading. In a case of
    a simple bus like :class:`Bus` class, every component is tried to read from and the first component which responds
    to the provided address "wins" and its value is taken for the next processing.


System
========
After you create required components, you may combine them into a system. To create a component, you need to inherit
from :class:`System` class. Then you implement five methods:

* :func:`System::doClocks`: here you define what to do when a main clock signal is sent to your system.
* :func:`System::doSteps`: here you proceed as many clock as needed to process a single CPU instruction (if CPU available, otherwise leave blank).
* :func:`System::doFrames`: here you proceed as many clock as needed to process a single video frame (if video output available, otherwise leave blank).
* :func:`System::doRun`: this is called when the user wants to keep the emulation running in the real time.
* A constructor of your class.

In the constructor you need to define several things:

* :member:`System::m_systemName`: system's name,
* :member:`System::m_systemClockRate`: system's main clock rate,
* interconnect your components,
* push all the components to the base class container: ``m_components.push_back(&m_myComponent);``,
* if there is any audio output, push it to the sample sources: ``m_sampleSources.push_back(myComponent.getSoundSampleSources());``

To implement :func:`Component::doRun()` you can use this template to properly calculate how many clock to proceed:

.. code-block:: cpp

    void mySystem::doRun(unsigned int updateFrequency) {
        // Calculate how many clocks to run based on function call interval.
        unsigned int remainingClocks = m_systemClockRate / updateFrequency;
        doClocks(remainingClocks);
    }

Interconnecting the components
********************************
You can check out the NES systems constructor in NES.cpp to see an example.


Adding the system to the plaform
===================================
When the system is finished, you can integrate it to the rest of the platform.

At first, in the Emulator.h, add a new value to the :member:`Emulator::SYSTEMS` enum.
Then, in Emulator.cpp, include your new system at the top of the file: ``#include "systems/mySystem.h"`` and
add a new row to the menu in :func:`Emulator::guiMenuItems()` like this:

.. code-block:: cpp

    // ... other systems ...
    else if(ImGui::MenuItem("mySystem", nullptr, m_systemID == SYSTEMS::MYSYSTEM)) {

                loadSystem(std::make_unique<MYSYSTEM>());
                m_systemID = SYSTEMS::MYSYSTEM;
    }

🎉 THAT'S IT 🎉
================
Congratulations, you now have a working emulation of your system. Have fun!

If everything is good, do not forget to leave a star ⭐ at GitHub. If something isn't working 🪲, please create an issue.