/****************************************************************************
** Small meta object code from reading C++ file 'small.h'
**
** Created: Tue Apr 15 12:27:07 2003
**      by: The Qt MOC ($Id: small.moc.cpp,v 1.1 2003/05/06 16:13:34 erisod-8 Exp $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "small.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.1.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *Small::className() const
{
    return "Small";
}

QMetaObject *Small::metaObj = 0;
static QMetaObjectCleanUp cleanUp_Small( "Small", &Small::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString Small::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Small", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString Small::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Small", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* Small::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KMainWindow::staticMetaObject();
    static const QUMethod slot_0 = {"monsterKnappen", 0, 0 };
    static const QUMethod slot_1 = {"saveInstance", 0, 0 };
    static const QUMethod slot_2 = {"timerDone", 0, 0 };
    static const QUParameter param_slot_3[] = {
	{ "selected", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_3 = {"profileSelected", 1, param_slot_3 };
    static const QUParameter param_slot_4[] = {
	{ "selected", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_4 = {"contextSelected", 1, param_slot_4 };
    static const QMetaData slot_tbl[] = {
	{ "monsterKnappen()", &slot_0, QMetaData::Public },
	{ "saveInstance()", &slot_1, QMetaData::Public },
	{ "timerDone()", &slot_2, QMetaData::Public },
	{ "profileSelected(int)", &slot_3, QMetaData::Public },
	{ "contextSelected(int)", &slot_4, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"Small", parentObject,
	slot_tbl, 5,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_Small.setMetaObject( metaObj );
    return metaObj;
}

void* Small::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "Small" ) )
	return this;
    return KMainWindow::qt_cast( clname );
}

bool Small::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: monsterKnappen(); break;
    case 1: saveInstance(); break;
    case 2: timerDone(); break;
    case 3: profileSelected((int)static_QUType_int.get(_o+1)); break;
    case 4: contextSelected((int)static_QUType_int.get(_o+1)); break;
    default:
	return KMainWindow::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool Small::qt_emit( int _id, QUObject* _o )
{
    return KMainWindow::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool Small::qt_property( int id, int f, QVariant* v)
{
    return KMainWindow::qt_property( id, f, v);
}

bool Small::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
