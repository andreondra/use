Developer's Guide
##################

.. toctree::
   :maxdepth: 2
   :caption: Contents:

This guide describes the structure and inner workings of the platform. After reading this guide, you
will be able to create your own components and systems.

.. note::
   Class names are also links to their respective documentation; just click 🔘👈 them to be redirected.
   Also, if you understand Czech, you can read a `bachelor's thesis <https://github.com/andreondra/bachelor-thesis>`_
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

Port and Connector
===================
When two components communicate, usually there is a component who controls the communication (active) and the other component
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
you can use functions which offers your selected port type, e.g. ``m_mainBus.read()``, ``m_mainBus.write()`` or ``m_INT.send()``.
Check :class:`Port` for more information.

Connector
***********
If you want to be your component controlled by any other, you have to specify a :class:`Connector`.