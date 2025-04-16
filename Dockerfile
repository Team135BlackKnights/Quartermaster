FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install Apache, CGI support, and build tools
RUN apt-get update && apt-get install -y \
    apache2 \
    libapache2-mod-fcgid \
    g++ \
    make \
    libmysqlclient-dev \
    graphviz \
    mysql-client \
    && rm -rf /var/lib/apt/lists/*
#Install Python3, set python3 as default
RUN apt-get update && apt-get install -y python3 python3-pip
RUN ln -s /usr/bin/python3 /usr/bin/python
#Install Pytnon dependencies
#Update pip
RUN pip3 install --upgrade pip
RUN pip3 install matplotlib numpy


RUN mkdir -p /var/www/html/cgi-bin

# Working directory
WORKDIR /app

# Copy source code
COPY cgi-bin /usr/lib/cgi-bin

# Build the C++ project (produces "parts")
RUN cd /usr/lib/cgi-bin && make

# Now copy the parts binary and ensure it's executable
RUN cp /usr/lib/cgi-bin/parts /usr/lib/cgi-bin/parts.cgi && chmod +x /usr/lib/cgi-bin/parts.cgi

# Now copy the images to the right location
RUN cp /usr/lib/cgi-bin/asm.png /var/www/html/ && cp /usr/lib/cgi-bin/part.png /var/www/html/
#Setup CGI
RUN a2enmod cgid
RUN echo "ScriptAlias /cgi-bin/ /usr/lib/cgi-bin/" >> /etc/apache2/apache2.conf
RUN echo "<Directory \"/usr/lib/cgi-bin\">\n\
    AllowOverride None\n\
    Options +ExecCGI -MultiViews +SymLinksIfOwnerMatch\n\
    Require all granted\n\
</Directory>" >> /etc/apache2/apache2.conf
COPY cgi-bin /usr/lib/cgi-bin
RUN chmod +x /usr/lib/cgi-bin/parts.cgi
# Expose Apache port
EXPOSE 80

# Start Apache
CMD ["apachectl", "-D", "FOREGROUND"]
