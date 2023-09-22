#include "throttledhttp.h"

ThrottledHttp::ThrottledHttp(Http &http) : http(http), milliseconds(1000) {
    elapsedTimer.start();
}

HttpReply *ThrottledHttp::request(const HttpRequest &req) {
    return new ThrottledHttpReply(http, req, milliseconds, elapsedTimer);
}

ThrottledHttpReply::ThrottledHttpReply(Http &http,
                                       const HttpRequest &req,
                                       int milliseconds,
                                       QElapsedTimer &elapsedTimer)
    : http(http), req(req), milliseconds(milliseconds), elapsedTimer(elapsedTimer), timer(nullptr) {
    checkElapsed();
}

void ThrottledHttpReply::checkElapsed() {
    /*
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    */

    const qint64 elapsedSinceLastRequest = elapsedTimer.elapsed();
    if (elapsedSinceLastRequest < milliseconds) {
        if (!timer) {
            timer = new QTimer(this);
            timer->setSingleShot(true);
            timer->setTimerType(Qt::PreciseTimer);
            connect(timer, &QTimer::timeout, this, &ThrottledHttpReply::checkElapsed);
        }
        // qDebug() << "Throttling" << req.url << QStringLiteral("%1ms").arg(milliseconds -
        // elapsedSinceLastRequest);
        timer->setInterval(milliseconds - elapsedSinceLastRequest);
        timer->start();
        return;
    }
    elapsedTimer.start();
    doRequest();
}

void ThrottledHttpReply::doRequest() {
    auto reply = http.request(req);
    connect(reply, &HttpReply::data, this, &HttpReply::data);
    connect(reply, &HttpReply::error, this, &HttpReply::error);
    connect(reply, &HttpReply::finished, this, &HttpReply::finished);

    // this will cause the deletion of this object once the request is finished
    setParent(reply);
}
