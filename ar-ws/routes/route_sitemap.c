#include "../routes.h"
#include <stdio.h>
#include <string.h>

void sitemap_handler(ClientConnection *conn, HttpRequest *req) {
    const char *host = "alrigroup.com";

    char xml[2048];
    int offset = 0;

    offset += snprintf(xml + offset, sizeof(xml) - offset,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");

    offset += snprintf(xml + offset, sizeof(xml) - offset,
        "  <url>\n"
        "    <loc>https://%s/</loc>\n"
        "    <lastmod>2026-07-01</lastmod>\n"
        "    <changefreq>monthly</changefreq>\n"
        "    <priority>1.0</priority>\n"
        "  </url>\n", host);

    offset += snprintf(xml + offset, sizeof(xml) - offset, "</urlset>\n");

    server_send_response(conn, 200, "application/xml", xml);
}
