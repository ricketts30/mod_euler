mod_euler README file

This is a small apache2 module that I use for doing the euler problems
(http://www.projecteuler.net/)

I was going to do the Euler problems as a command-line app, but suddenly 
thought that an Apache module, written in C would be much more fun and 
useful.

Note: I am developing on an Apple iMac (running OS X 10.5.8).  At the 
moment the build instructions are pretty specific to that environment.

I intend to add some of the following:
* startup
* configuration
* teardown
* add caching instructions
* better documentation :-)

Configuration
~~~~~~~~~~~~~

Note: You can do this before compile and install

At the end of the httpd.conf file you will need to add the following:

<Location /euler-info>
SetHandler euler-handler
</Location>

AddHandler euler-handler .euler

# the single piece of config - if it can't open the file
# it returns some pre-coded CSS instead.
EulerStylesheetFilePath "/etc/apache2/mod_euler.css"

Compile and Install
~~~~~~~~~~~~~~~~~~~

open a terminal session and navigate to the mod_euler directory
type the following:
 
  make euler
  
(you will be asked for an administration password)
then type:

  sudo apachectl -k graceful

Now you can go to http://localhost/test.euler/ and you should see the 
thing work!
