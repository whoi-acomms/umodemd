/****************************************************************************
** ProcessParser meta object code from reading C++ file 'streamexec.h'
**
** Created: Sat May 10 13:29:27 2003
**      by: The Qt MOC ($Id: streamexec.moc.cpp,v 1.1 2003/05/13 15:26:22 marjuh-8 Exp $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "streamexec.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.1.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *ProcessParser::className() const
{
    return "ProcessParser";
}

QMetaObject *ProcessParser::metaObj = 0;
static QMetaObjectCleanUp cleanUp_ProcessParser( "ProcessParser", &ProcessParser::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString ProcessParser::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ProcessParser", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString ProcessParser::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ProcessParser", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* ProcessParser::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QObject::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "proc", &static_QUType_ptr, "KProcess", QUParameter::In }
    };
    static const QUMethod slot_0 = {"streamExited", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "proc", &static_QUType_ptr, "KProcess", QUParameter::In },
	{ "buffer", &static_QUType_charstar, 0, QUParameter::In },
	{ "buflen", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"receivedStdout", 3, param_slot_1 };
    static const QMetaData slot_tbl[] = {
	{ "streamExited(KProcess*)", &slot_0, QMetaData::Public },
	{ "receivedStdout(KProcess*,char*,int)", &slot_1, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"ProcessParser", parentObject,
	slot_tbl, 2,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_ProcessParser.setMetaObject( metaObj );
    return metaObj;
}

void* ProcessParser::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "ProcessParser" ) )
	return this;
    return QObject::qt_cast( clname );
}

bool ProcessParser::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: streamExited((KProcess*)static_QUType_ptr.get(_o+1)); break;
    case 1: receivedStdout((KProcess*)static_QUType_ptr.get(_o+1),(char*)static_QUType_charstar.get(_o+2),(int)static_QUType_int.get(_o+3)); break;
    default:
	return QObject::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool ProcessParser::qt_emit( int _id, QUObject* _o )
{
    return QObject::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool ProcessParser::qt_property( int id, int f, QVariant* v)
{
    return QObject::qt_property( id, f, v);
}

bool ProcessParser::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
