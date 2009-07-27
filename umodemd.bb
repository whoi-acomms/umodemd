inherit autotools

DESCRIPTION="WHOI Micro-modem daemon"

PR="r0"

DEPENDS="libnmea"

SRC_URI="http://acomms.whoi.edu/software/${PN}/${P}.tar.gz"

FILES_${PN} = "${bindir}/umodemd"

RDEPENDS_${PN} = "libnmea"
