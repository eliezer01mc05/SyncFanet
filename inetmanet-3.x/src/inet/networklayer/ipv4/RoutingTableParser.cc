//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

//  Cleanup and rewrite: Andras Varga, 2004

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "inet/networklayer/ipv4/RoutingTableParser.h"

#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"

namespace inet {

//
// Constants
//
const int MAX_FILESIZE = 10000;    // TBD lift hardcoded limit
const int MAX_ENTRY_STRING_SIZE = 500;

//
// Tokens that mark the route file.
//
const char *IFCONFIG_START_TOKEN = "ifconfig:",
*IFCONFIG_END_TOKEN = "ifconfigend.",
*ROUTE_START_TOKEN = "route:",
*ROUTE_END_TOKEN = "routeend.",
*RULES_START_TOKEN = "rules:",
*RULES_END_TOKEN = "rulesend.";

int RoutingTableParser::streq(const char *str1, const char *str2)
{
    return strncmp(str1, str2, strlen(str2)) == 0;
}

int RoutingTableParser::strcpyword(char *dest, const char *src)
{
    int i;
    for (i = 0; !isspace(dest[i] = src[i]); i++)
        ;
    dest[i] = '\0';
    return i;
}

void RoutingTableParser::skipBlanks(char *str, int& charptr)
{
    for ( ; isspace(str[charptr]); charptr++)
        ;
}

int RoutingTableParser::readRoutingTableFromFile(const char *filename)
{
    FILE *fp;
    int charpointer;
    char *file = new char[MAX_FILESIZE];
    char *ifconfigFile = nullptr;
    char *routeFile = nullptr;
    int c;

    fp = fopen(filename, "r");
    if (fp == nullptr)
        throw cRuntimeError("Error opening routing table file `%s'", filename);

    // read the whole into the file[] char-array
    for (charpointer = 0; (c = getc(fp)) != EOF; charpointer++)
        file[charpointer] = c;

    for ( ; charpointer < MAX_FILESIZE; charpointer++)
        file[charpointer] = '\0';

    fclose(fp);

    // copy file into specialized, filtered char arrays
    for (charpointer = 0;
         (charpointer < MAX_FILESIZE) && (file[charpointer] != '\0');
         charpointer++)
    {
        // check for tokens at beginning of file or line
        if (charpointer == 0 || file[charpointer - 1] == '\n') {
            // copy into ifconfig filtered chararray
            if (streq(file + charpointer, IFCONFIG_START_TOKEN)) {
                ifconfigFile = createFilteredFile(file,
                            charpointer,
                            IFCONFIG_END_TOKEN);
                //PRINTF("Filtered File 1 created:\n%s\n", ifconfigFile);
            }

            // copy into route filtered chararray
            if (streq(file + charpointer, ROUTE_START_TOKEN)) {
                routeFile = createFilteredFile(file,
                            charpointer,
                            ROUTE_END_TOKEN);
                //PRINTF("Filtered File 2 created:\n%s\n", routeFile);
            }
        }
    }

    delete[] file;

    // parse filtered files
    if (ifconfigFile)
        parseInterfaces(ifconfigFile);
    if (routeFile)
        parseRouting(routeFile);

    delete[] ifconfigFile;
    delete[] routeFile;

    return 0;
}

char *RoutingTableParser::createFilteredFile(char *file, int& charpointer, const char *endtoken)
{
    int i = 0;
    char *filterFile = new char[MAX_FILESIZE];
    filterFile[0] = '\0';

    while (true) {
        // skip blank lines and comments
        while (!isalnum(file[charpointer]) && !isspace(file[charpointer])) {
            while (file[charpointer++] != '\n')
                ;
        }

        // check for endtoken:
        if (streq(file + charpointer, endtoken)) {
            filterFile[i] = '\0';
            break;
        }

        // copy whole line to filterFile
        while ((filterFile[i++] = file[charpointer++]) != '\n')
            ;
    }

    return filterFile;
}

void RoutingTableParser::parseInterfaces(char *ifconfigFile)
{
    char buf[MAX_ENTRY_STRING_SIZE];
    int charpointer = 0;
    InterfaceEntry *ie = nullptr;

    // parsing of entries in interface definition
    while (ifconfigFile[charpointer] != '\0') {
        // name entry
        if (streq(ifconfigFile + charpointer, "name:")) {
            // find existing interface with this name
            char *name = parseEntry(ifconfigFile, "name:", charpointer, buf);
            ie = ift->getInterfaceByName(name);
            if (!ie)
                throw cRuntimeError("Error in routing file: interface name `%s' not registered by any L2 module", name);
            if (!ie->ipv4Data())
                throw cRuntimeError("Error in routing file: interface name `%s' doesn't have IPv4 data fields", name);

            continue;
        }

        // encap entry
        if (streq(ifconfigFile + charpointer, "encap:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            // ignore encap
            parseEntry(ifconfigFile, "encap:", charpointer, buf);
            continue;
        }

        // HWaddr entry
        if (streq(ifconfigFile + charpointer, "HWaddr:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            // ignore hwAddr
            parseEntry(ifconfigFile, "HWaddr:", charpointer, buf);
            continue;
        }

        // inet_addr entry
        if (streq(ifconfigFile + charpointer, "inet_addr:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            ie->ipv4Data()->setIPAddress(IPv4Address(parseEntry(ifconfigFile, "inet_addr:", charpointer, buf)));
            continue;
        }

        // Broadcast address entry
        if (streq(ifconfigFile + charpointer, "Bcast:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            // ignore Bcast
            parseEntry(ifconfigFile, "Bcast:", charpointer, buf);
            continue;
        }

        // Mask entry
        if (streq(ifconfigFile + charpointer, "Mask:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            ie->ipv4Data()->setNetmask(IPv4Address(parseEntry(ifconfigFile, "Mask:", charpointer, buf)));
            continue;
        }

        // Multicast groups entry
        if (streq(ifconfigFile + charpointer, "Groups:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            char *grStr = parseEntry(ifconfigFile, "Groups:", charpointer, buf);
            parseMulticastGroups(grStr, ie);
            continue;
        }

        // MTU entry
        if (streq(ifconfigFile + charpointer, "MTU:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            ie->setMtu(atoi(parseEntry(ifconfigFile, "MTU:", charpointer, buf)));
            continue;
        }

        // Metric entry
        if (streq(ifconfigFile + charpointer, "Metric:")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            ie->ipv4Data()->setMetric(atoi(parseEntry(ifconfigFile, "Metric:", charpointer, buf)));
            continue;
        }

        // BROADCAST Flag
        if (streq(ifconfigFile + charpointer, "BROADCAST")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            ie->setBroadcast(true);
            charpointer += strlen("BROADCAST");
            skipBlanks(ifconfigFile, charpointer);
            continue;
        }

        // MULTICAST Flag
        if (streq(ifconfigFile + charpointer, "MULTICAST")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            ie->setMulticast(true);
            charpointer += strlen("MULTICAST");
            skipBlanks(ifconfigFile, charpointer);
            continue;
        }

        // POINTTOPOINT Flag
        if (streq(ifconfigFile + charpointer, "POINTTOPOINT")) {
            if (!ie)
                throw cRuntimeError("Error in routing file: missing the `name:' entry");
            ie->setPointToPoint(true);
            charpointer += strlen("POINTTOPOINT");
            skipBlanks(ifconfigFile, charpointer);
            continue;
        }

        // no entry discovered: move charpointer on
        charpointer++;
    }
}

char *RoutingTableParser::parseEntry(char *ifconfigFile, const char *tokenStr,
        int& charpointer, char *destStr)
{
    int temp = 0;

    charpointer += strlen(tokenStr);
    skipBlanks(ifconfigFile, charpointer);
    temp = strcpyword(destStr, ifconfigFile + charpointer);
    charpointer += temp;

    skipBlanks(ifconfigFile, charpointer);

    return destStr;
}

void RoutingTableParser::parseMulticastGroups(char *groupStr, InterfaceEntry *itf)
{
    // Parse string (IPv4 addresses separated by colons)
    cStringTokenizer tokenizer(groupStr, ":");
    const char *token;
    while ((token = tokenizer.nextToken()) != nullptr)
        itf->ipv4Data()->joinMulticastGroup(IPv4Address(token));
}

void RoutingTableParser::parseRouting(char *routeFile)
{
    char *str = new char[MAX_ENTRY_STRING_SIZE];

    int pos = strlen(ROUTE_START_TOKEN);
    skipBlanks(routeFile, pos);
    while (routeFile[pos] != '\0') {
        // 1st entry: Host
        pos += strcpyword(str, routeFile + pos);
        skipBlanks(routeFile, pos);
        IPv4Route *e = new IPv4Route();
        if (strcmp(str, "default:")) {
            // if entry is not the default entry
            if (!IPv4Address::isWellFormed(str))
                throw cRuntimeError("Syntax error in routing file: `%s' on 1st column should be `default:' or a valid IPv4 address", str);

            e->setDestination(IPv4Address(str));
        }

        // 2nd entry: Gateway
        pos += strcpyword(str, routeFile + pos);
        skipBlanks(routeFile, pos);
        if (!strcmp(str, "*") || !strcmp(str, "0.0.0.0")) {
            e->setGateway(IPv4Address::UNSPECIFIED_ADDRESS);
        }
        else {
            if (!IPv4Address::isWellFormed(str))
                throw cRuntimeError("Syntax error in routing file: `%s' on 2nd column should be `*' or a valid IPv4 address", str);

            e->setGateway(IPv4Address(str));
        }

        // 3rd entry: Netmask
        pos += strcpyword(str, routeFile + pos);
        skipBlanks(routeFile, pos);
        if (!IPv4Address::isWellFormed(str))
            throw cRuntimeError("Syntax error in routing file: `%s' on 3rd column should be a valid IPv4 address", str);

        e->setNetmask(IPv4Address(str));

        // 4th entry: flags
        pos += strcpyword(str, routeFile + pos);
        skipBlanks(routeFile, pos);
        // parse flag-String to set flags
        for (int i = 0; str[i]; i++) {
            if (str[i] == 'H') {
                // e->setType(IPv4Route::DIRECT);
            }
            else if (str[i] == 'G') {
                // e->setType(IPv4Route::REMOTE);
            }
            else {
                throw cRuntimeError("Syntax error in routing file: 4th column should be `G' or `H' not `%s'", str);
            }
        }

        // 5th entry: metric
        pos += strcpyword(str, routeFile + pos);
        skipBlanks(routeFile, pos);
        int metric = atoi(str);
        if (metric == 0 && str[0] != '0')
            throw cRuntimeError("Syntax error in routing file: 5th column should be numeric not `%s'", str);

        e->setMetric(metric);

        // 6th entry: interface
        opp_string interfaceName;
        interfaceName.reserve(MAX_ENTRY_STRING_SIZE);
        pos += strcpyword(interfaceName.buffer(), routeFile + pos);
        skipBlanks(routeFile, pos);
        InterfaceEntry *ie = ift->getInterfaceByName(interfaceName.c_str());
        if (!ie)
            throw cRuntimeError("Syntax error in routing file: 6th column: `%s' is not an existing interface", interfaceName.c_str());

        e->setInterface(ie);
        e->setSourceType(IRoute::MANUAL);

        // add entry
        rt->addRoute(e);
    }
    delete [] str;
}


void RoutingTableParser::parseRules(char *rulesFile)
{
    char *str = new char[MAX_ENTRY_STRING_SIZE];

    int pos = strlen(RULES_START_TOKEN);
    skipBlanks(rulesFile, pos);


    while (rulesFile[pos] != '\0')
    {
        // 1st entry: Host
        pos += strcpyword(str, rulesFile + pos);
        skipBlanks(rulesFile, pos);
        if (!strcmp(str, "iptables"))
        {
             throw cRuntimeError("Syntax error in routing file: `%s' on 1st column should be `DROP' now is the only rule accepted", str);
        }
        int position=-1;
        IPv4RouteRule *e = new IPv4RouteRule();
        while (rulesFile[pos] != '\0')
        {
              pos += strcpyword(str, rulesFile + pos);
              skipBlanks(rulesFile, pos);
              if (!strcmp(str, "-A"))
              {
                   pos += strcpyword(str, rulesFile + pos);
                   skipBlanks(rulesFile, pos);
                   if (!strcmp(str, "INPUT"))
                       position = 1;
                   else if (!strcmp(str, "OUTPUT"))
                       position = 0;
                   else if (!strcmp(str, "FORWARD"))
                       position = 3;
                   else
                       throw cRuntimeError("Syntax error in routing file: `%s' should be INPUT or OUTPUT", str);
                   continue;
              }
              if (!strcmp(str, "-s"))
              {
                   pos += strcpyword(str, rulesFile + pos);
                   skipBlanks(rulesFile, pos);
                   // find mask
                   IPv4Address mask("255.255.255.255");
                   char * p = strstr(str,"/");
                   if (p != nullptr)
                   {
                       char strAux[30];
                       strcpy (strAux,p+1);
                       int size=atoi(strAux);
                       unsigned int m=1;
                       for (int i=0; i<size; i++)
                       {
                           m = (m<<1)|m;
                       }
                       IPv4Address aux(m);
                       mask = aux;
                       *p='\0';
                   }

                   if (!IPv4Address::isWellFormed(str))
                       throw cRuntimeError("Syntax error in routing file: `%s' should be a valid IPv4 address", str);
                   e->setSrcAddress(IPv4Address(str));
                   e->setSrcNetmask(mask);
                   continue;
              }
              if (!strcmp(str, "-d"))
              {
                   pos += strcpyword(str, rulesFile + pos);
                   skipBlanks(rulesFile, pos);
                   IPv4Address mask("255.255.255.255");
                   char * p = strstr(str,"/");
                   if (p!=nullptr)
                   {
                       char strAux[30];
                       strcpy (strAux,p+1);
                       int size = atoi(strAux);
                       unsigned int m = 1;
                       for (int i=0; i<size; i++)
                       {
                           m = (m<<1)|m;
                       }
                       IPv4Address aux(m);
                       mask = aux;
                       *p = '\0';
                   }
                   if (!IPv4Address::isWellFormed(str))
                       throw cRuntimeError("Syntax error in routing file: `%s' should be a valid IPv4 address", str);
                   e->setDestAddress(IPv4Address(str));
                   e->setDestNetmask(mask);
                   continue;
               }
              if (!strcmp(str, "-p"))
              {
                   pos += strcpyword(str, rulesFile + pos);
                   skipBlanks(rulesFile, pos);
                   if (!strcmp(str, "tcp"))
                       e->setProtocol(IP_PROT_TCP);
                   else if (!strcmp(str, "udp"))
                       e->setProtocol(IP_PROT_UDP);
                   else
                       throw cRuntimeError("Syntax error in routing file: `%s' should be tcp or udp", str);
                   continue;
              }
              if (!strcmp(str, "-j"))
              {
                   pos += strcpyword(str, rulesFile + pos);
                   skipBlanks(rulesFile, pos);
                   if (!strcmp(str, "DROP"))
                       e->setRule(IPv4RouteRule::DROP);
                   else if (!strcmp(str, "ACCEPT"))
                       e->setRule(IPv4RouteRule::ACCEPT);
                   else
                       throw cRuntimeError("Syntax error in routing file: `%s' should be DROP or ACCEPT", str);
                   continue;
              }
              if (!strcmp(str, "-sport"))
              {
                   pos += strcpyword(str, rulesFile + pos);
                   skipBlanks(rulesFile, pos);
                   int port = atoi(str);
                   e->setSrcPort(port);
                   continue;
              }
              if (!strcmp(str, "-dport"))
              {
                   pos += strcpyword(str, rulesFile + pos);
                   skipBlanks(rulesFile, pos);
                   int port = atoi(str);
                   e->setDestPort(port);
                   continue;
              }
              if (!strcmp(str, "-i"))
              {
                  pos += strcpyword(str, rulesFile + pos);
                  skipBlanks(rulesFile, pos);
                  InterfaceEntry *ie = ift->getInterfaceByName(str);
                  if (ie==nullptr)
                       throw cRuntimeError("Syntax error in routing file: `%s' should be a valid interface name", str);
                  else
                       e->setInterface(ie);
                  continue;
              }
              if (!strcmp(str, "iptables"))
              {
                  if (position == 0)
                      rt->addRule(true, e);
                  else if (position == 1)
                      rt->addRule(false, e);
                  else if (position == 3 && e->getRule() == IPv4RouteRule::ACCEPT && e->getInterface())
                      rt->addRule(false, e);
                  else
                     throw cRuntimeError("table rule not valid, must indicate input or output or \"FORWARD with accept\" ");
                  if (e->getRule()!=IPv4RouteRule::DROP && e->getRule()!=IPv4RouteRule::ACCEPT)
                     throw cRuntimeError("table rule not valid ");

                  position = -1;
                  e = new IPv4RouteRule();
                  continue;
              }
         }
         if (position==0)
             rt->addRule(true, e);
         else if (position==1)
             rt->addRule(false, e);
         else if (position==3 && e->getRule()==IPv4RouteRule::ACCEPT && e->getInterface())
             rt->addRule(false, e);
         else
             throw cRuntimeError("table rule not valid, must indicate input or output");
         if (e->getRule()!=IPv4RouteRule::DROP && e->getRule()!=IPv4RouteRule::ACCEPT)
             throw cRuntimeError("table rule not valid ");
    }
}


} // namespace inet

