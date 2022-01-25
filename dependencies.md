## Installation dependencies for ubuntu 20.04:

```
sudo apt-get install -q -y --no-install-recommends \  
    libglu1-mesa-dev \
    libtiff5-dev \
    qtbase5-dev \
    qtscript5-dev \
    libqt5svg5-dev \
    libqt5opengl5-dev \
    libqt5xmlpatterns5-dev \
    qtwebengine5-dev \
    qttools5-dev \
    libqt5charts5-dev \
    libqt5x11extras5-dev \
    qtxmlpatterns5-dev-tools \
    libqt5webengine-data \    
    libopenslide-dev 
```

## Installation dependencies for ubuntu 18.04:

```
sudo apt-get install -q -y --no-install-recommends \  
    libglu1-mesa-dev \
    libopenslide-dev 
```
To run M²aia on Ubuntu 18.04 zou have to provide the Qt libraries >= Qt 5.12.9.
Installers can be found in the [Qt 5.12] archive. In this example Qt is installed in the system folder ```/opt/``` folder to provide multi-user access.

To propergate the libraryes system wide, you can add a new entry

``` /etc/ld.so.conf.d/local_qt_5.12.conf ```

and add a single line e.g. pointing to

``` /opt/Qt5.12/5.12.9/gcc_64/lib ```

Finally you have to run ```ldconfig``` to promote the new library search path.


[Qt 5.12]: https://download.qt.io/archive/qt/5.12/
