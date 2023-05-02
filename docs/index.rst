.. image:: _static/logo-full-path.svg
  :width: 600

Universal System Emulator documentation
#######################################

Introduction
*************

Welcome to the Universal System Emulator documentation! 👋

The Universal System Emulator is mainly meant to be a platform for emulation of any historical system emulator.
There are three ways you can *use the USE*.

Use the USE as a base for an emulation project 🏭
=======================================
If you want to build your own emulator of any system, USE can be a good choice for a platform for the project.
With USE, you can focus on development of the emulated components, instead of losing time with setting up
the GUI library, audio library, thinking about how to interconnect the components and implement debugging, etc.
Everything is ready, you just add components and connect them into a system. If you implemented a new
system and you would like to add it to the USE project as an example system, you are welcome to open a PR. Please
keep the original project structure (new components place to the components folder, new systems to the systems folder, ...).
Check the :ref:`Developer's Guide`.

Use the USE as a user 🎮
======================
If you just want to play around and try to run some software on supported systems, you just need to download, (compile)
and run the executable. Check the :ref:`User Manual`.

Contribute to the USE 💌
=====================
If you miss any functionality of the platform (e.g. more sophisticated sound interface), you may open an issue or
you can add it yourself and open a PR. If there won't be any problems, I will happily merge the changed to the upstream 😁.
:ref:`Contributor's Guide`

Contents
*******************
* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

Table of Contents
^^^^^^^^^^^^^^^^^
.. toctree::
    :maxdepth: 2

    self
    dev_manual
    user_manual
    contr_manual
    _exhale/api_docs_root
