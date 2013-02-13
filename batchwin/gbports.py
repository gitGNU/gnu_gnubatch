#   Copyright 2013 Free Software Foundation, Inc.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

import xmlutil

class gbports:
    """Represents port numbers used by system for easy editing"""

    def __init__(self):

        self.client_access = 48106
        self.connect_tcp = 48104
        self.connect_udp = 48104
        self.jobview = 48105
        self.api_tcp = 48107
        self.api_udp = 48107

    def load(self, parent):
        """Load saved port numbers from XML file"""

        node = parent.firstChild()
        while not node.isNull():
            tagn = node.toElement().tagName()
            if tagn == "CONN_TCP":
                self.connect_tcp = int(xmlutil.getText(node))
            elif tagn == "CONN_UDP":
                self.connect_udp = int(xmlutil.getText(node))
            elif tagn == "CLIENT_ACCESS":
                self.client_access = int(xmlutil.getText(node))
            elif tagn == "JOBVIEW":
                self.jobview = int(xmlutil.getText(node))
            elif tagn == "API_TCP":
                self.api_tcp = int(xmlutil.getText(node))
            elif tagn == "API_UDP":
                self.api_udp = int(xmlutil.getText(node))
            node = node.nextSibling()

    def saveport(self, doc, ports, name, portnum):
        """Save a port number"""
        el = doc.createElement(name)
        txt = doc.createTextNode(str(portnum))
        el.appendChild(txt)
        ports.appendChild(el)

    def save(self, doc, parent, name):
        """Save port numbers to config XML file"""

        ports = doc.createElement(name);
        parent.appendChild(ports)
        self.saveport(doc, ports, "CONN_TCP", self.connect_tcp)
        self.saveport(doc, ports, "CONN_UDP", self.connect_udp)
        self.saveport(doc, ports, "CLIENT_ACCESS", self.client_access)
        self.saveport(doc, ports, "JOBVIEW", self.jobview)
        self.saveport(doc, ports, "API_TCP", self.api_tcp)
        self.saveport(doc, ports, "API_UDP", self.api_udp)
