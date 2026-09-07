#//define IS_PRODUCTION 1

#include <QCoreApplication>
#include "ssl_server.h"
#include "wifi_tcp_server.h"
#include <locale.h>

int main(int argc, char *argv[])
{
    // Set C locale for MPV (required by libmpv)
    setlocale(LC_NUMERIC, "C");
    
    QCoreApplication app(argc, argv);
    
    // QCoreApplication may change locale, so set it again
    setlocale(LC_NUMERIC, "C");

    // Start SSL Server on port 8765 (any interface)
    SslServer wifiServer;
    
    if (!wifiServer.listen(QHostAddress::Any, 8765)) {
        qCritical() << "SSL Server could not start!";
        return 1;
    }

    qDebug() << "SSL Server is running on WIFI port 8765";

    // Start Ethernet TCP Server on 10.0.0.100:7358 (no SSL)
    WiFiTcpServer ethServer;
    
    #ifdef IS_PRODUCTION
    if (!ethServer.listen(QHostAddress("10.0.0.100"), 7358)) {
        qCritical() << "Ethernet TCP Server could not start!";
        qCritical() << "Error:" << ethServer.errorString();
        return 1;
    }
    qDebug() << "Ethernet TCP Server is running on ETHERNET 10.0.0.100:7358 (no SSL)";
    #endif

    

    return app.exec();
}
