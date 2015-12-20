This program takes a URL to an article as a command line argument. It parses the
host name from the URL, converts to an IP address, and creates a TCP connection
using a socket. It sends an HTTP request to the server for the file containing
the article and then reads the response one chunk at a time. It looks for any
text between HTML <p> tags, removing any other HTML tags in between. It writes
the text to a file in the current directory with the same name as the article.
You can then view the article in Vim or whatever other distraction-free program
you prefer.

Note: this only works for URLs where the host name is the actual host of the
file (i.e., no redirection occurs), which is not the case for some of the
larger news sites. Some hosts that did work for me: www.huffingtonpost.com,
www.theonion.com, espn.go.com.
