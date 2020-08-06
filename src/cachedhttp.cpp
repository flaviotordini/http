#include "cachedhttp.h"
#include "localcache.h"

CachedHttpReply::CachedHttpReply(const QByteArray &body, const HttpRequest &req)
    : bytes(body), req(req) {
    QTimer::singleShot(0, this, SLOT(emitSignals()));
}

QByteArray CachedHttpReply::body() const {
    return bytes;
}

void CachedHttpReply::emitSignals() {
    emit data(body());
    emit finished(*this);
    deleteLater();
}

WrappedHttpReply::WrappedHttpReply(LocalCache *cache, const QByteArray &key, HttpReply *httpReply)
    : HttpReply(httpReply), cache(cache), key(key), httpReply(httpReply) {
    connect(httpReply, SIGNAL(data(QByteArray)), SIGNAL(data(QByteArray)));
    connect(httpReply, SIGNAL(error(QString)), SIGNAL(error(QString)));
    connect(httpReply, SIGNAL(finished(HttpReply)), SLOT(originFinished(HttpReply)));
}

void WrappedHttpReply::originFinished(const HttpReply &reply) {
    qDebug() << reply.statusCode() << reply.url();
    if (reply.isSuccessful()) cache->insert(key, reply.body());
    emit finished(reply);
}

CachedHttp::CachedHttp(Http &http, const char *name)
    : http(http), cache(LocalCache::instance(name)), cachePostRequests(false) {}

void CachedHttp::setMaxSeconds(uint seconds) {
    cache->setMaxSeconds(seconds);
}

void CachedHttp::setMaxSize(uint maxSize) {
    cache->setMaxSize(maxSize);
}

HttpReply *CachedHttp::request(const HttpRequest &req) {
    bool cacheable = req.operation == QNetworkAccessManager::GetOperation ||
                     (cachePostRequests && req.operation == QNetworkAccessManager::PostOperation);
    if (!cacheable) {
        qDebug() << "Not cacheable" << req.url;
        return http.request(req);
    }
    const QByteArray key = requestHash(req);
    const QByteArray value = cache->value(key);
    if (!value.isNull()) {
        qDebug() << "HIT" << key << req.url;
        return new CachedHttpReply(value, req);
    }
    qDebug() << "MISS" << key << req.url;
    return new WrappedHttpReply(cache, key, http.request(req));
}

QByteArray CachedHttp::requestHash(const HttpRequest &req) {
    const char sep = '|';

    QByteArray s;
    if (ignoreHostname) {
        s = (req.url.scheme() + sep + req.url.path() + sep + req.url.query()).toUtf8();
    } else {
        s = req.url.toEncoded();
    }
    s += sep + req.body + sep + QByteArray::number(req.offset);
    if (req.operation == QNetworkAccessManager::PostOperation) {
        s.append(sep);
        s.append("POST");
    }
    return LocalCache::hash(s);
}
