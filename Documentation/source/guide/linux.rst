Linux
=====

.. toctree::
   :maxdepth: 2

This chapter will guide your through the installation and maintenance of Natron on Linux.

Requirements
------------

Natron will work on any distribution released after 2010, this includes (but not limited to):

 * CentOS/RHEL 6.4 and higher
 * Fedora 14 and higher
 * Ubuntu 10.04 and higher
 * Debian 7 and higher

The basic requirements are:

 * Linux 2.6.18 and higher
 * Glibc 2.12 and higher
 * libgcc 4.4 and higher
 * Working X11 and OpenGL (Mesa/Nvidia/Catalyst)

Download
--------

Navigate to http://natron.fr/download and download the latest version. This documentation will assume that you downloaded the installer (our default and recommended choice).

.. image:: _images/linux_install_01.png

Extract
-------

When the file has been downloaded, extract the file. This can be done in your file browser, usually just right-click and select 'Extract Here'.

.. image:: _images/linux_install_02.png

Install
-------

You are now ready to start the installation, double-click on the extracted file to start the installation.

.. image:: _images/linux_install_04.png

*On some installations you are not allowed to execute downloaded files, right-click and select properties, then tick the 'Execute file as program' option. This option may have a different name depending on your distribution and desktop environment. You can also make the file executable through the terminal, type chmod +x filename.*

You should now be greated with the installation wizard.

.. image:: _images/linux_install_05.png

Click 'Next' to start the installation, you first option is where to install Natron. Usually the default location is good enough. If you select a installation path outside your home directory you will need to supply the root (administrator) password before you can continue.

.. image:: _images/linux_install_06.png

Your next option is the package selection, most users should accept the default. Each package has an more in-depth description if you want to know what they provide.

.. image:: _images/linux_install_07.png

Then comes the standard license agreement, Natron and it's plug-ins are licensed under the GPL version 2. You can read more about the licenses for each component included in Natron after installation (in Help=>About).

.. image:: _images/linux_install_08.png

The installation wizard is now ready to install Natron on your computer. The process should not take more than a minute or two (depending on your computer).

.. image:: _images/linux_install_09.png

The installation is now over! Start Natron and enjoy.

.. image:: _images/linux_install_10.png

Natron can be started from the desktop menu (under Graphics) or by executing the 'Natron' file in the folder you installed Natron.

.. image:: _images/linux_install_11.png
.. image:: _images/linux_install_12.png

Maintenance
-----------

Natron includes a maintenance tool called 'NatronSetup', with this application you can easily upgrade Natron and it's components when a new version is available. You can also add or remove individual packages, or remove Natron completely. The application is in the 'Graphics' section in the desktop menu, or you can start it from the folder where you installed Natron.

.. image:: _images/linux_install_11.png
.. image:: _images/linux_install_13.png

.. image:: _images/linux_install_14.png

The application also include a basic settings category, where you can configure proxy and other advanced options.

Advanced installation
---------------------

Natron also has RPM and DEB packages, these are recommended for multi-user installations or for deployment on more than one machine. You can find more information on our website at http://natron.fr/download .
