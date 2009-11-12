# Copyright 1999-2009 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI="2"

inherit kde4-base

DESCRIPTION="Quadkonsole provides a grid of Konsole terminals."
HOMEPAGE="http://nomis80.org/quadkonsole/"
SRC_URI="http://nomis80.org/${PN}/${P}.tar.lzma"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64"
IUSE=""

RDEPEND="
	>=kde-base/konsole-${KDE_MINIMAL}
"
DEPEND="
	${RDEPEND}
"

