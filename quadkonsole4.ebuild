# Copyright 1999-2009 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=3
inherit kde4-base

MY_P=${PN}4-${PV}

DESCRIPTION="Quadkonsole provides a grid of Konsole terminals"
HOMEPAGE="http://kb.ccchl.de/quadkonsole4/"
SRC_URI="http://kb.ccchl.de/${PN}4/${MY_P}.tar.lzma"

LICENSE="GPL-2"
SLOT="4"
KEYWORDS="~amd64 ~x86"
IUSE="debug"

RDEPEND="
	$(add_kdebase_dep konsole)
"
DEPEND="${RDEPEND}
	app-arch/xz-utils"

DOCS=( AUTHORS ChangeLog )

S=${WORKDIR}/${MY_P}
