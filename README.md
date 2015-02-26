# qt for ARM Compilation 

** This was done on an ODROID XU3 which is an 8-core with 2GB of ram. 

It is recommended that you add swap space to avoid 'out of memory' crashes

Unfortunately, by default, certain typedefs in the QT4 package compile as 'float' in ARM (as opposed to 'double' in x86) - RGBDSLAM_V2 depends on these typedefs being 'double' so we either have two choices: figure out a code change in the RGBDSLAM_V2 package or recompile QT4 for ARM with the typedef 'qreal' set to 'double'. I haven't figured out the former first (and don't have any other dependencies on QT4) so here are instructions for the latter path:
First, retrieve the qt4 source files:

Basically the folder structure looks like the following:
** source/install/build directories should all be on the same level
Home
> qt
> > qt_source
> > > All of the stuff that is cloned from git
> > qt_install
> > qt_build
Cd
mkdir qt
git clone git://gitorious.org/qt/qt.git
mv qt qt_source
mkdir qt_install
mkdir qt_build 
cd qt_source
Once finished, configure and build:
cd qt_build
../qt_source/configure -prefix ../qt_install -nomake tests -nomake examples -D QT_COORD_TYPE=double

**We will be using the open source version and of course we will accept the terms. 
By executing 'configure' from an external folder (qt_build, in our case), we are doing a shadow build. This allows you to direct your build files to another folder so you don't blend everything with your source files. If something screws up, you just delete your build folder and don't have to git clone the source again (which takes a while).
The 'prefix' argument tells the build process where your installation files will go when you call make install. With similar reasoning for the shadow build, we redirect them to another folder 'qt_install'. We'll come back to that later. The 'D' argument tells QT that we are defining an internal constant / variable - this case, 'QT_COORD_TYPE'. We are setting it to 'double' - this will cause the 'qreal' typedef to be compiled as 'double'.
Once the configuration process finishes:
make
make install
This will take a while. When it's done, we need to copy the files to their proper locations in your OS. At this point, you may want to install 'libqt4' from a package manager (ala 'sudo apt-get'). If there are any other installed packages that need QT4 as a dependency, your package manager won't know that you've installed from source and may require you to install it's version. If you do this, it will overwrite any files you've manually copied. What you should do is install QT4 from your package manager, and then overwrite the files with your compiled version.
If you've installed QT4 through a package manager first, copy each folder except 'include' and 'plugins' from ~/qt_install to /usr/share/qt4. Copy those two excluded folders to /usr/include/qt4 and/usr/lib/qt4, respectively. If you did not already install QT, do the following:
sudo cp -a ~/qt_install/* /usr/share/qt4/
sudo mv ~/usr/share/qt4/include /usr/include/qt4
sudo mv ~/usr/share/qt4/plugins /usr/lib/qt4/plugins
sudo mv ~/usr/share/qt4/imports /usr/lib/qt4/imports
sudo mv ~/usr/share/qt4/lib/* /usr/lib/
Next, you need to create symbolic links from your /usr/share/qt4 library to where you copied yourinclude and plugins directories to. If you installed QT4 through the package manager first, these links already exist and you're done. Otherwise, use the ln command to do this.
