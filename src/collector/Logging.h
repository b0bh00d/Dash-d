#pragma once

#include <QDebug>

#define MSGLOGGER QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC)
#define NOQUOTE_NOSPACE .noquote().nospace
#undef qDebug
#define qDebug MSGLOGGER .debug() NOQUOTE_NOSPACE
#undef qInfo
#define qInfo MSGLOGGER .info() NOQUOTE_NOSPACE
#undef qWarning
#define qWarning MSGLOGGER .warning() NOQUOTE_NOSPACE
#undef qCritical
#define qCritical MSGLOGGER .critical() NOQUOTE_NOSPACE
