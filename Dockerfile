FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install system packages and Apache modules
RUN apt-get update && apt-get install -y \
    apache2 \
    apache2-utils \     
    libapache2-mod-fcgid \
    g++ \
    make \
    libmysqlclient-dev \
    graphviz \
    mysql-client \
    python3 python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Link `python` to `python3`
RUN ln -s /usr/bin/python3 /usr/bin/python

# Upgrade pip and install Python packages
RUN pip3 install --upgrade pip
RUN pip3 install matplotlib numpy

# Prepare directories
RUN mkdir -p /usr/lib/cgi-bin /var/www/html

# Copy CGI source code and build
COPY cgi-bin /usr/lib/cgi-bin
RUN cd /usr/lib/cgi-bin && make && cp parts parts.cgi && chmod +x parts parts.cgi

# Set executable permissions for CGI
RUN chmod +x /usr/lib/cgi-bin/parts /usr/lib/cgi-bin/parts.cgi

# Copy images to web root
RUN cp /usr/lib/cgi-bin/asm.png /var/www/html/ && cp /usr/lib/cgi-bin/part.png /var/www/html/

# Enable CGI module
RUN a2enmod cgid

# Set Apache configuration for CGI and .htaccess
RUN echo "ScriptAlias /cgi-bin/ /usr/lib/cgi-bin/" >> /etc/apache2/apache2.conf && \
    echo "<Directory \"/usr/lib/cgi-bin\">\n\
    AllowOverride All\n\
    Options +ExecCGI -MultiViews +SymLinksIfOwnerMatch\n\
    Require all granted\n\
</Directory>" >> /etc/apache2/apache2.conf && \
    echo "<Directory \"/var/www/html\">\n\
    AllowOverride All\n\
    Require all granted\n\
</Directory>" >> /etc/apache2/apache2.conf

RUN htpasswd -b -c /usr/lib/cgi-bin/.htpasswd admin changeme
RUN echo "AuthType Basic\n\
AuthName \"Restricted Area\"\n\
AuthUserFile /usr/lib/cgi-bin/.htpasswd\n\
Require valid-user" > /usr/lib/cgi-bin/.htaccess

# ✅ Redirect from `/` to `/cgi-bin/parts.cgi` using index.html
RUN echo '<!DOCTYPE html><html><head><meta http-equiv="refresh" content="0; URL=/cgi-bin/parts.cgi"></head></html>' > /var/www/html/index.html
RUN chown www-data:www-data /usr/lib/cgi-bin/.htpasswd
RUN chmod 664 /usr/lib/cgi-bin/.htpasswd
# Expose port 80
EXPOSE 80

# Start Apache in foreground
CMD ["apachectl", "-D", "FOREGROUND"]
