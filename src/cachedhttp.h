#ifndef CACHEDHTTP_H
#define CACHEDHTTP_H

#include "http.h"

class LocalCache;

class CachedHttp : public Http {
public:
    CachedHttp(Http &http = Http::instance(), const char *name = "http");
    void setMaxSeconds(uint seconds);
    void setMaxSize(uint maxSize);
    void setCachePostRequests(bool value) { cachePostRequests = value; }
    HttpReply *request(const HttpRequest &req);

private:
    Http &http;
    LocalCache *cache;
    bool cachePostRequests;
};

class CachedHttpReply : public HttpReply {
    Q_OBJECT

public:
    CachedHttpReply(const QByteArray &body, const HttpRequest &req);
    QUrl url() const { return req.url; }
    int statusCode() const { return 200; }
    QByteArray body() const;

private slots:
    void emitSignals();

private:
    const QByteArray bytes;
    const HttpRequest req;
};

class WrappedHttpReply : public HttpReply {
    Q_OBJECT

public:
    WrappedHttpReply(LocalCache *cache, const QByteArray &key, HttpReply *httpReply);
    QUrl url() const { return httpReply->url(); }
    int statusCode() const { return httpReply->statusCode(); }
    QByteArray body() const { return httpReply->body(); }

private slots:
    void originFinished(const HttpReply &reply);

private:
    LocalCache *cache;
    QByteArray key;
    HttpReply *httpReply;
};

#endif // CACHEDHTTP_H
