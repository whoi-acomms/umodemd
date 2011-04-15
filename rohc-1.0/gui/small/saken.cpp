#include <klocale.h>
/****************************************************************************
** Form implementation generated from reading ui file './saken.ui'
**
** Created: Tue Apr 22 10:52:07 2003
**      by: The User Interface Compiler ($Id: saken.cpp,v 1.1 2003/05/06 16:13:33 erisod-8 Exp $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "saken.h"

#include <qvariant.h>
#include "SignalPlotter.h"
#include <qtabwidget.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qlistbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>

static const char* const img0_saken[] = { 
"22 22 233 2",
"Qt c None",
".# c #000000",
".o c #000000",
".a c #000000",
".b c #000000",
".c c #000000",
".d c #000000",
".e c #000000",
".f c #7b7b7b",
".X c #000000",
".F c #000000",
".n c #000000",
"#n c #000000",
"#C c #000000",
"#Q c #000000",
".W c #000000",
"#a c #000000",
"#m c #000000",
"#B c #000000",
"#P c #000000",
"bM c #000000",
".g c #a1a1a1",
".m c #060606",
".E c #000000",
"bL c #000000",
".h c #939393",
".l c #383838",
".V c #000000",
"bx c #000000",
"## c #000000",
".4 c #020202",
"#y c #020204",
"#q c #020206",
"#p c #020208",
"#d c #02020a",
".3 c #02020c",
".5 c #02020d",
"aU c #040202",
"au c #040206",
"#S c #04020c",
"aV c #040404",
"aN c #04040c",
"#x c #04040d",
"a6 c #08060c",
"aG c #0a0808",
"#e c #0d0f10",
"#T c #101012",
"aF c #101213",
"ap c #121212",
"bd c #13150c",
"aH c #13150d",
"#9 c #151612",
"aT c #151812",
"bf c #151d0f",
"bK c #161904",
"#M c #161912",
"an c #161c13",
"#h c #181c13",
"aq c #1a1a16",
"bc c #1a1c10",
"ag c #1c1d16",
"ar c #1c2210",
"aE c #1c2212",
"be c #1e2513",
"ay c #212616",
"a7 c #222715",
".6 c #222815",
"#0 c #282d13",
"a2 c #283015",
"#D c #2a2f13",
"bw c #2a3107",
"bJ c #2a3108",
".2 c #2b3115",
"#8 c #2b3315",
"#X c #2c321a",
"#4 c #2f331a",
"#f c #2f3418",
"ad c #313918",
"#g c #313a19",
".D c #374109",
"at c #383f19",
"a5 c #383f1a",
"#Y c #404c18",
"aB c #424e19",
"#E c #454e1c",
"#r c #454f1d",
".U c #4c590c",
"#i c #4f5b1d",
"a1 c #50591d",
"a3 c #505b1d",
"a# c #505c1d",
"ao c #516120",
"bz c #525f0f",
"#l c #52600d",
"a4 c #53611e",
"bA c #53620d",
"by c #53620e",
"aj c #536221",
"#J c #53631e",
"#. c #54620e",
"bh c #55630e",
"bI c #556310",
"bB c #56640f",
"aW c #566520",
"#O c #57650f",
"bC c #59680f",
"#c c #59681c",
"#2 c #5a690f",
"az c #5a6922",
"bD c #5b6a10",
"#w c #5b6a22",
"ab c #5b6b11",
"ak c #5c6a22",
"bE c #5d6c13",
"aY c #5d6d0e",
"bG c #5d6d10",
"al c #5d6d13",
"bp c #5e6a22",
"aa c #5e6b1e",
"aJ c #5e6d0f",
"bF c #5e6d11",
".k c #5f5f5f",
"bo c #5f6a20",
"aw c #5f6e11",
"bH c #617110",
"a9 c #617210",
"#L c #647322",
"aO c #647423",
"#3 c #64761a",
"ac c #657420",
".N c #657520",
".M c #65761a",
"bb c #667227",
"#z c #667326",
".7 c #677922",
"aM c #6a7622",
".j c #6d6d6d",
"#o c #6e7d20",
".1 c #6e7d21",
"bn c #768822",
"am c #768921",
"bq c #778728",
"#1 c #778823",
"av c #778828",
"#R c #778921",
".O c #778925",
".L c #778a21",
"aC c #788925",
".i c #7a7a7a",
".Z c #88a213",
"#s c #899c20",
"#b c #89a016",
".I c #89a018",
"af c #8ca326",
".H c #8da321",
".K c #8da41e",
"#I c #8da426",
"aS c #8fa332",
"as c #8fa431",
"aD c #90a42c",
"ah c #90a62a",
".J c #90a916",
".0 c #90a91c",
"ax c #91a91d",
".P c #91aa1e",
"#W c #91aa22",
"#7 c #92a926",
"bg c #92ab29",
"bu c #92ab2a",
".C c #94a53e",
"bv c #96af1d",
"#v c #96b028",
"aZ c #97b01a",
".R c #97b119",
"bi c #97b11a",
".S c #97b21a",
"bt c #97b22a",
".T c #98b11a",
"a8 c #98b22b",
"#G c #99b415",
"#U c #99b512",
"#5 c #9ab121",
"#t c #9ab221",
"bm c #9bb028",
"#F c #9bb322",
"bj c #9bb51c",
"aL c #9cb51a",
".Q c #9cb61a",
".9 c #9cb71a",
"#N c #9db12a",
"b# c #9db519",
"#V c #9db815",
"#6 c #9db915",
"ae c #9db91e",
".8 c #9eb825",
"#H c #9eba13",
"#u c #9eba1e",
".G c #9fb046",
"aA c #9fb92b",
"bk c #9fba1d",
"br c #a0b827",
"#K c #a0ba2c",
"#k c #a0bb19",
"a0 c #a1b425",
"aI c #a1b927",
"ba c #a1ba21",
"#j c #a1bb20",
"aP c #a3bd2c",
"#Z c #a3bd2d",
"bl c #a9c223",
"#A c #aac428",
"aQ c #adcb22",
"a. c #aecc25",
".s c #afbe63",
".t c #b0bf64",
".u c #b1c164",
"aR c #b2d41d",
"ai c #b2d420",
".r c #b4c170",
"b. c #b4c55d",
".B c #b4c65d",
".Y c #b5c170",
".v c #b5c564",
".q c #b7c18b",
"aK c #b7c664",
".z c #b7c764",
".w c #b7c765",
".A c #b7c865",
".p c #b8bca7",
".y c #b8c865",
".x c #b8c965",
"bs c #bdde25",
"aX c #bede23",
"QtQtQt.#.a.b.c.d.e.e.e.e.e.e.e.e.d.c.b.a.#Qt",
"QtQt.f.g.h.i.j.j.j.j.j.j.j.j.j.j.k.l.m.n.c.o",
"Qt.#.g.p.q.r.s.t.u.v.w.x.y.z.z.A.B.C.D.E.F.c",
"Qt.a.h.q.G.H.I.J.K.L.M.N.O.P.Q.R.S.T.U.V.W.X",
"Qt.b.i.Y.H.Z.0.1.2.3.4.4.5.6.7.8.9.Q#.###a.n",
"Qt.c.j.s#b.0#c#d.3#e#f#g#h.3#d#i#j#k#l###m#n",
"Qt.d.j.t.J#o#p#q#r#s#t#u#v#w#x#y#z#A#.###B#C",
"Qt.e.j.u.K#D#y#E#F#G#H#I#J#K#L#p#M#N#O###P#Q",
"Qt.e.j.v#R#S#T#s#U#V#W#X#p#Y#Z#0#y#1#2###P#Q",
"Qt.e.j.w#3#y#4#5#6#7#8#p#9.Oa.a##yaaab###P#Q",
"Qt.e.j.yac.4adaeaf#X#p#qagahaiaj#yakal###P#Q",
"Qt.e.j.xam.5an#vao#papaq#qarasatauavaw###P#Q",
"Qt.e.j.waxay.3azaAaBaCaDaE.4aFaGaHaIaJ###P#Q",
"Qt.e.jaKaLaM.3aNaOaPaQaRaSaTaUaVaWaXaY###P#Q",
"Qt.e.j.waZa0a1.4#da2a3a4a5a6aVaVa7a8a9###P#Q",
"Qt.d.kb..Rb#babbbc#q#y.4#ybdaWbebfbgbh###B#C",
"Qt.c.l.Cbibjbkblbmbnbobpbqbrbsbtbubvbwbx#m#n",
"Qt.b.m.D.UbybzbAbBbCbDbEbFbGaYbHbIbJbKbL#a.n",
"Qt.a.n.E.V########################bxbLbM.W.X",
"Qt.#.c.F.W#a#m#B#P#P#P#P#P#P#P#P#B#m#a.W.F.c",
"QtQt.o.c.X.n#n#C#Q#Q#Q#Q#Q#Q#Q#Q#C#n.n.X.c.o",
"QtQtQt.#.a.b.c.d.e.e.e.e.e.e.e.e.d.c.b.a.#Qt"};


/* 
 *  Constructs a Form1 as a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f'.
 */
Form1::Form1( QWidget* parent, const char* name, WFlags fl )
    : QWidget( parent, name, fl ),
      image0( (const char **) img0_saken )
{
    if ( !name )
	setName( "Form1" );
    setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, sizePolicy().hasHeightForWidth() ) );
    setMinimumSize( QSize( 650, 710 ) );
    Form1Layout = new QGridLayout( this, 1, 1, 11, 6, "Form1Layout"); 

    tabWidget3 = new QTabWidget( this, "tabWidget3" );

    tab = new QWidget( tabWidget3, "tab" );

    textLabel1 = new QLabel( tab, "textLabel1" );
    textLabel1->setGeometry( QRect( 10, 20, 77, 20 ) );

    textLabel2 = new QLabel( tab, "textLabel2" );
    textLabel2->setGeometry( QRect( 10, 60, 121, 20 ) );

    textLabel3 = new QLabel( tab, "textLabel3" );
    textLabel3->setGeometry( QRect( 10, 100, 77, 20 ) );

    textLabel4 = new QLabel( tab, "textLabel4" );
    textLabel4->setGeometry( QRect( 10, 140, 216, 20 ) );

    textLabel5 = new QLabel( tab, "textLabel5" );
    textLabel5->setGeometry( QRect( 10, 180, 179, 20 ) );

    textLabel6 = new QLabel( tab, "textLabel6" );
    textLabel6->setGeometry( QRect( 10, 220, 174, 20 ) );

    textLabel7 = new QLabel( tab, "textLabel7" );
    textLabel7->setGeometry( QRect( 10, 260, 80, 20 ) );

    textLabel8 = new QLabel( tab, "textLabel8" );
    textLabel8->setGeometry( QRect( 10, 300, 77, 20 ) );

    textLabel9 = new QLabel( tab, "textLabel9" );
    textLabel9->setGeometry( QRect( 10, 340, 97, 20 ) );

    textLabel10 = new QLabel( tab, "textLabel10" );
    textLabel10->setGeometry( QRect( 270, 20, 290, 20 ) );

    textLabel12 = new QLabel( tab, "textLabel12" );
    textLabel12->setGeometry( QRect( 270, 100, 290, 20 ) );

    textLabel11 = new QLabel( tab, "textLabel11" );
    textLabel11->setGeometry( QRect( 270, 60, 290, 20 ) );

    textLabel14 = new QLabel( tab, "textLabel14" );
    textLabel14->setGeometry( QRect( 270, 180, 290, 20 ) );

    textLabel15 = new QLabel( tab, "textLabel15" );
    textLabel15->setGeometry( QRect( 270, 220, 290, 20 ) );

    textLabel13 = new QLabel( tab, "textLabel13" );
    textLabel13->setGeometry( QRect( 270, 140, 290, 20 ) );

    checkBox1 = new QCheckBox( tab, "checkBox1" );
    checkBox1->setGeometry( QRect( 270, 340, 106, 24 ) );

    comboBox1 = new QComboBox( FALSE, tab, "comboBox1" );
    comboBox1->setGeometry( QRect( 270, 380, 105, 32 ) );

    textLabel1_2 = new QLabel( tab, "textLabel1_2" );
    textLabel1_2->setGeometry( QRect( 10, 380, 134, 20 ) );

    pushButton1 = new QPushButton( tab, "pushButton1" );
    pushButton1->setGeometry( QRect( 10, 480, 380, 32 ) );

    textLabel2_2 = new QLabel( tab, "textLabel2_2" );
    textLabel2_2->setGeometry( QRect( 10, 420, 156, 20 ) );

    radioButton1 = new QRadioButton( tab, "radioButton1" );
    radioButton1->setGeometry( QRect( 70, 550, 140, 24 ) );

    lineEdit3 = new QLineEdit( tab, "lineEdit3" );
    lineEdit3->setGeometry( QRect( 70, 590, 123, 24 ) );

    lineEdit2 = new QLineEdit( tab, "lineEdit2" );
    lineEdit2->setGeometry( QRect( 220, 550, 120, 24 ) );

    pushButton2 = new QPushButton( tab, "pushButton2" );
    pushButton2->setGeometry( QRect( 408, 258, 160, 190 ) );

    spinBox2 = new QSpinBox( tab, "spinBox2" );
    spinBox2->setGeometry( QRect( 270, 260, 60, 22 ) );
    spinBox2->setMaxValue( 10000 );

    spinBox3 = new QSpinBox( tab, "spinBox3" );
    spinBox3->setGeometry( QRect( 270, 300, 60, 24 ) );
    spinBox3->setMaxValue( 5000 );

    spinBox5 = new QSpinBox( tab, "spinBox5" );
    spinBox5->setGeometry( QRect( 270, 420, 60, 22 ) );
    tabWidget3->insertTab( tab, QString("") );

    tab_2 = new QWidget( tabWidget3, "tab_2" );

    groupBox2 = new QGroupBox( tab_2, "groupBox2" );
    groupBox2->setGeometry( QRect( 10, 260, 570, 230 ) );

    textLabel24 = new QLabel( groupBox2, "textLabel24" );
    textLabel24->setGeometry( QRect( 490, -200, 86, 20 ) );

    textLabel20 = new QLabel( groupBox2, "textLabel20" );
    textLabel20->setGeometry( QRect( 10, 150, 156, 20 ) );

    textLabel19 = new QLabel( groupBox2, "textLabel19" );
    textLabel19->setGeometry( QRect( 10, 110, 90, 20 ) );

    textLabel18 = new QLabel( groupBox2, "textLabel18" );
    textLabel18->setGeometry( QRect( 10, 70, 121, 20 ) );

    textLabel17 = new QLabel( groupBox2, "textLabel17" );
    textLabel17->setGeometry( QRect( 10, 30, 112, 20 ) );

    textLabel23 = new QLabel( groupBox2, "textLabel23" );
    textLabel23->setGeometry( QRect( 270, 110, 270, 20 ) );

    textLabel22 = new QLabel( groupBox2, "textLabel22" );
    textLabel22->setGeometry( QRect( 270, 70, 270, 20 ) );

    textLabel21 = new QLabel( groupBox2, "textLabel21" );
    textLabel21->setGeometry( QRect( 270, 30, 270, 20 ) );

    spinBox4 = new QSpinBox( groupBox2, "spinBox4" );
    spinBox4->setGeometry( QRect( 270, 150, 56, 24 ) );
    spinBox4->setMaxValue( 5000 );

    textLabel16 = new QLabel( tab_2, "textLabel16" );
    textLabel16->setGeometry( QRect( 10, 10, 107, 20 ) );

    listBox2 = new QListBox( tab_2, "listBox2" );
    listBox2->setGeometry( QRect( 10, 40, 570, 210 ) );
    tabWidget3->insertTab( tab_2, QString("") );

    tab_3 = new QWidget( tabWidget3, "tab_3" );

    textLabel25 = new QLabel( tab_3, "textLabel25" );
    textLabel25->setGeometry( QRect( 10, 10, 269, 20 ) );

    groupBox4 = new QGroupBox( tab_3, "groupBox4" );
    groupBox4->setGeometry( QRect( 10, 130, 570, 510 ) );

    textLabel26 = new QLabel( groupBox4, "textLabel26" );
    textLabel26->setGeometry( QRect( 20, 30, 274, 20 ) );

    textLabel27 = new QLabel( groupBox4, "textLabel27" );
    textLabel27->setGeometry( QRect( 20, 60, 330, 20 ) );

    textLabel28 = new QLabel( groupBox4, "textLabel28" );
    textLabel28->setGeometry( QRect( 20, 90, 340, 20 ) );

    textLabel29 = new QLabel( groupBox4, "textLabel29" );
    textLabel29->setGeometry( QRect( 20, 120, 332, 20 ) );

    textLabel30 = new QLabel( groupBox4, "textLabel30" );
    textLabel30->setGeometry( QRect( 20, 150, 297, 20 ) );

    textLabel31 = new QLabel( groupBox4, "textLabel31" );
    textLabel31->setGeometry( QRect( 20, 180, 353, 20 ) );

    textLabel32 = new QLabel( groupBox4, "textLabel32" );
    textLabel32->setGeometry( QRect( 20, 200, 385, 39 ) );

    textLabel33 = new QLabel( groupBox4, "textLabel33" );
    textLabel33->setGeometry( QRect( 20, 240, 391, 20 ) );

    textLabel48 = new QLabel( groupBox4, "textLabel48" );
    textLabel48->setGeometry( QRect( 460, 210, 86, 20 ) );

    textLabel47 = new QLabel( groupBox4, "textLabel47" );
    textLabel47->setGeometry( QRect( 460, 180, 86, 20 ) );

    textLabel46 = new QLabel( groupBox4, "textLabel46" );
    textLabel46->setGeometry( QRect( 460, 150, 86, 20 ) );

    textLabel51 = new QLabel( groupBox4, "textLabel51" );
    textLabel51->setGeometry( QRect( 460, 300, 86, 20 ) );

    textLabel50 = new QLabel( groupBox4, "textLabel50" );
    textLabel50->setGeometry( QRect( 460, 270, 86, 20 ) );

    textLabel44 = new QLabel( groupBox4, "textLabel44" );
    textLabel44->setGeometry( QRect( 460, 90, 86, 20 ) );

    textLabel43 = new QLabel( groupBox4, "textLabel43" );
    textLabel43->setGeometry( QRect( 460, 60, 86, 20 ) );

    textLabel49 = new QLabel( groupBox4, "textLabel49" );
    textLabel49->setGeometry( QRect( 460, 240, 86, 20 ) );

    textLabel45 = new QLabel( groupBox4, "textLabel45" );
    textLabel45->setGeometry( QRect( 460, 120, 86, 20 ) );

    textLabel52 = new QLabel( groupBox4, "textLabel52" );
    textLabel52->setGeometry( QRect( 460, 330, 86, 20 ) );

    textLabel53 = new QLabel( groupBox4, "textLabel53" );
    textLabel53->setGeometry( QRect( 460, 360, 86, 20 ) );

    textLabel54 = new QLabel( groupBox4, "textLabel54" );
    textLabel54->setGeometry( QRect( 460, 390, 86, 20 ) );

    textLabel55 = new QLabel( groupBox4, "textLabel55" );
    textLabel55->setGeometry( QRect( 460, 420, 86, 20 ) );

    textLabel56 = new QLabel( groupBox4, "textLabel56" );
    textLabel56->setGeometry( QRect( 460, 450, 86, 20 ) );

    textLabel57 = new QLabel( groupBox4, "textLabel57" );
    textLabel57->setGeometry( QRect( 460, 480, 86, 20 ) );

    textLabel34 = new QLabel( groupBox4, "textLabel34" );
    textLabel34->setGeometry( QRect( 20, 270, 167, 20 ) );

    textLabel35 = new QLabel( groupBox4, "textLabel35" );
    textLabel35->setGeometry( QRect( 20, 300, 185, 20 ) );

    textLabel36 = new QLabel( groupBox4, "textLabel36" );
    textLabel36->setGeometry( QRect( 20, 330, 430, 20 ) );

    textLabel37 = new QLabel( groupBox4, "textLabel37" );
    textLabel37->setGeometry( QRect( 20, 360, 430, 20 ) );

    textLabel38 = new QLabel( groupBox4, "textLabel38" );
    textLabel38->setGeometry( QRect( 20, 390, 420, 20 ) );

    textLabel39 = new QLabel( groupBox4, "textLabel39" );
    textLabel39->setGeometry( QRect( 20, 420, 430, 20 ) );

    textLabel40 = new QLabel( groupBox4, "textLabel40" );
    textLabel40->setGeometry( QRect( 20, 450, 430, 20 ) );

    textLabel41 = new QLabel( groupBox4, "textLabel41" );
    textLabel41->setGeometry( QRect( 20, 480, 430, 20 ) );

    textLabel42 = new QLabel( groupBox4, "textLabel42" );
    textLabel42->setGeometry( QRect( 460, 30, 86, 20 ) );

    listBox3 = new QListBox( tab_3, "listBox3" );
    listBox3->setGeometry( QRect( 10, 40, 570, 78 ) );
    tabWidget3->insertTab( tab_3, QString("") );

    tab_4 = new QWidget( tabWidget3, "tab_4" );

    signalPlotter1 = new SignalPlotter( tab_4, "signalPlotter1" );
    signalPlotter1->setGeometry( QRect( 70, 110, 20, 20 ) );
    tabWidget3->insertTab( tab_4, QString("") );

    tab_5 = new QWidget( tabWidget3, "tab_5" );
    tabWidget3->insertTab( tab_5, QString("") );

    Form1Layout->addWidget( tabWidget3, 0, 0 );
    languageChange();
    resize( QSize(650, 710).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
Form1::~Form1()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void Form1::languageChange()
{
    setCaption( tr2i18n( "ROHC Performance and Benchmarking application" ) );
    textLabel1->setText( tr2i18n( "Creator" ) );
    textLabel2->setText( tr2i18n( "Version number" ) );
    textLabel3->setText( tr2i18n( "Status" ) );
    textLabel4->setText( tr2i18n( "Number of compressed flows" ) );
    textLabel5->setText( tr2i18n( "Total number of packets" ) );
    textLabel6->setText( tr2i18n( "Total compression ratio" ) );
    textLabel7->setText( tr2i18n( "MAX_CID" ) );
    textLabel8->setText( tr2i18n( "MRRU" ) );
    textLabel9->setText( tr2i18n( "LARGE_CID" ) );
    textLabel10->setText( tr2i18n( "textLabel10" ) );
    textLabel12->setText( tr2i18n( "textLabel12" ) );
    textLabel11->setText( tr2i18n( "textLabel11" ) );
    textLabel14->setText( tr2i18n( "textLabel14" ) );
    textLabel15->setText( tr2i18n( "textLabel15" ) );
    textLabel13->setText( tr2i18n( "textLabel13" ) );
    checkBox1->setText( tr2i18n( "Enabled" ) );
    comboBox1->clear();
    comboBox1->insertItem( tr2i18n( "56k" ) );
    comboBox1->insertItem( tr2i18n( "ISDN" ) );
    comboBox1->insertItem( tr2i18n( "xDSL" ) );
    comboBox1->insertItem( tr2i18n( "T1" ) );
    comboBox1->insertItem( tr2i18n( "T3" ) );
    textLabel1_2->setText( tr2i18n( "Connection type: " ) );
    pushButton1->setText( tr2i18n( "Starta en timer" ) );
    textLabel2_2->setText( tr2i18n( "Feedback frequency" ) );
    radioButton1->setText( tr2i18n( "radioButton1" ) );
    pushButton2->setText( tr2i18n( "Save settings" ) );
    tabWidget3->changeTab( tab, tr2i18n( "ROHC Instance table" ) );
    groupBox2->setTitle( tr2i18n( "groupBox2" ) );
    textLabel24->setText( tr2i18n( "textLabel24" ) );
    textLabel20->setText( tr2i18n( "Feedback frequency" ) );
    textLabel19->setText( tr2i18n( "Profile type" ) );
    textLabel18->setText( tr2i18n( "Version number" ) );
    textLabel17->setText( tr2i18n( "Profile number" ) );
    textLabel23->setText( tr2i18n( "textLabel23" ) );
    textLabel22->setText( tr2i18n( "textLabel22" ) );
    textLabel21->setText( tr2i18n( "textLabel21" ) );
    textLabel16->setText( tr2i18n( "List of profiles" ) );
    tabWidget3->changeTab( tab_2, tr2i18n( "ROHC Profile table" ) );
    textLabel25->setText( tr2i18n( "Context type, CID, CID state, Profile" ) );
    groupBox4->setTitle( tr2i18n( "groupBox4" ) );
    textLabel26->setText( tr2i18n( "Total compression ratio of all packets" ) );
    textLabel27->setText( tr2i18n( "Total compression ratio of all packet headers" ) );
    textLabel28->setText( tr2i18n( "Mean compressed package size of all packets" ) );
    textLabel29->setText( tr2i18n( "Mean header size of all compressed headers" ) );
    textLabel30->setText( tr2i18n( "Compression ratio of the last 16 packets" ) );
    textLabel31->setText( tr2i18n( "Compression ratio of the last 16 packet headers" ) );
    textLabel32->setText( tr2i18n( "Mean compressed packet size of the last 16 packets" ) );
    textLabel33->setText( tr2i18n( "Mean header size of the last 16 compressed headers" ) );
    textLabel48->setText( tr2i18n( "textLabel48" ) );
    textLabel47->setText( tr2i18n( "textLabel47" ) );
    textLabel46->setText( tr2i18n( "textLabel46" ) );
    textLabel51->setText( tr2i18n( "textLabel51" ) );
    textLabel50->setText( tr2i18n( "textLabel50" ) );
    textLabel44->setText( tr2i18n( "textLabel44" ) );
    textLabel43->setText( tr2i18n( "textLabel43" ) );
    textLabel49->setText( tr2i18n( "textLabel49" ) );
    textLabel45->setText( tr2i18n( "textLabel45" ) );
    textLabel52->setText( tr2i18n( "textLabel52" ) );
    textLabel53->setText( tr2i18n( "textLabel53" ) );
    textLabel54->setText( tr2i18n( "textLabel54" ) );
    textLabel55->setText( tr2i18n( "textLabel55" ) );
    textLabel56->setText( tr2i18n( "textLabel56" ) );
    textLabel57->setText( tr2i18n( "textLabel57" ) );
    textLabel34->setText( tr2i18n( "Context activation time" ) );
    textLabel35->setText( tr2i18n( "Context deactivation time" ) );
    textLabel36->setText( tr2i18n( "textLabel36" ) );
    textLabel37->setText( tr2i18n( "textLabel37" ) );
    textLabel38->setText( tr2i18n( "textLabel38" ) );
    textLabel39->setText( tr2i18n( "textLabel39" ) );
    textLabel40->setText( tr2i18n( "textLabel40" ) );
    textLabel41->setText( tr2i18n( "textLabel41" ) );
    textLabel42->setText( tr2i18n( "textLabel42" ) );
    tabWidget3->changeTab( tab_3, tr2i18n( "ROHC Context table" ) );
    tabWidget3->changeTab( tab_4, tr2i18n( "Graph" ) );
    tabWidget3->changeTab( tab_5, tr2i18n( "Tab" ) );
}

#include "saken.moc"
