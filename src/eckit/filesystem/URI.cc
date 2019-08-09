/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <iostream>
#include <vector>

#include "eckit/exception/Exceptions.h"
#include "eckit/filesystem/URI.h"
#include "eckit/filesystem/URIManager.h"
#include "eckit/serialisation/Stream.h"
#include "eckit/utils/StringTools.h"
#include "eckit/utils/Tokenizer.h"


namespace eckit {

URIParts::URIParts(const std::string& str) {
    size_t first = 0;
    size_t last = str.length();

    // get fragment start
    std::size_t fragmentStart = str.find_last_of("#");
    if (fragmentStart != std::string::npos) {
        fragment_ = str.substr(fragmentStart + 1);

        last = fragmentStart;
    }

    // get query start
    std::size_t queryStart = str.find_last_of("?", last);
    if (queryStart != std::string::npos) {
        std::string query = str.substr(queryStart + 1, last - queryStart - 1);
        last = queryStart;

        if (!query.empty())
            parseQueryValues(query);
    }

    if (1 < last && str[first] == '/' && str[first+1] == '/') {
        first += 2;

        std::size_t userEnd = str.find_last_of("@", last);
        if (userEnd != std::string::npos && userEnd>first) {
            user_ = str.substr(first, userEnd-first);
            first = userEnd + 1;
        }

        std::size_t portStart = str.find(":", first);
        if (portStart != std::string::npos && portStart < last) {
            ASSERT(portStart > 0);
            host_ = str.substr(first, portStart-first);
            port_ = 0;
            int i=portStart+1;
            for (;i<last && std::isdigit(str[i]); i++) {
                port_ = port_*10 + (str[i] - '0');
            }
            first = i;
        } else {
            port_ = -1;
            std::size_t hostEnd = str.find("/", first);
            ASSERT(hostEnd != std::string::npos);
            host_ = str.substr(first, hostEnd-first);
            first = hostEnd;
        }
    }

    path_ = str.substr(first, last-first);
}

void URIParts::parseQueryValues(const std::string &query) {

    Tokenizer parse("&");
    Tokenizer splitValues("=");
    std::vector<std::string> attributeValuePairs;

    parse(query, attributeValuePairs);
    for (std::string &attributeValue : attributeValuePairs) {
        std::vector<std::string> s;
        splitValues(attributeValue, s);
        if (s.size() == 2) {
            queryValues_[s[0]] = s[1];
        }
    }
}

std::string URIParts::authority() const {
    std::string authority;
    if (!user_.empty())
        authority = user_ + "@";
    if (!host_.empty())
        authority += host_;
    if (port_ != -1)
        authority += ":" + std::to_string(port_);

    return authority;
}

void URIParts::query(const std::string& attribute, const std::string& value) {
    queryValues_[attribute] = value;
}


const std::string URIParts::query(const std::string& attribute) const {
    auto it = queryValues_.find(attribute);
    if (it != queryValues_.end())
        return it->second;
    return "";
}

std::string URIParts::query() const {
    std::string query;
    for (auto &attributeValue : queryValues_) {
        if (!query.empty())
            query += "&";

        query += attributeValue.first + "=" + attributeValue.second;
    }
    return query;
}

//----------------------------------------------------------------------------------------------------------------------

URI::URI() {}

URI::URI(const std::string& uri) {
    if (!uri.empty())
        parseScheme(uri);
}

URI::URI(const URI &uri, const std::string &scheme):
    scheme_(scheme), name_(uri.name_) {}

URI::URI(const URI &uri, const std::string &scheme, const std::string &host, const int port):
    scheme_(scheme), name_(uri.name_) {
    parts()->host(host);
    parts()->port(port);
}


URI::URI(Stream &s) {
    std::string query;

    s >> scheme_;
    s >> name_;

/*    s >> host_;
    s >> port_;
    s >> path_;
    s >> query;
    s >> fragment_;

    if (!query.empty())
        parseQueryValues(query);*/
}

URI::~URI() {}

void URI::parseScheme(const std::string& uri) {
    std::size_t schemeEnd = uri.find(":");
    if (schemeEnd != std::string::npos) {
        std::string schemeLowercase = StringTools::lower(uri.substr(0, schemeEnd));
        std::size_t acceptableProtocolEnd = schemeLowercase.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789+-.");
        if (acceptableProtocolEnd == std::string::npos) {
            scheme_ = uri.substr(0, schemeEnd);
            name_   = uri.substr(schemeEnd + 1);
        }
    }
    if (name_.empty()) {
        name_ = std::string(uri);
    }

    if (scheme_.empty())
        scheme_ = "unix";
}

bool URI::exists() {
    ASSERT(!name_.empty());
    ASSERT(!scheme_.empty());
    return URIManager::lookUp(scheme_).exists(*this);
}

DataHandle* URI::newWriteHandle() {
    ASSERT(!name_.empty());
    ASSERT(!scheme_.empty());
    return URIManager::lookUp(scheme_).newWriteHandle(*this);
}

DataHandle* URI::newReadHandle(const OffsetList& ol, const LengthList& ll) {
    ASSERT(!name_.empty());
    ASSERT(!scheme_.empty());
    return URIManager::lookUp(scheme_).newReadHandle(*this, ol, ll);
}

DataHandle* URI::newReadHandle() {
    ASSERT(!name_.empty());
    ASSERT(!scheme_.empty());
    return URIManager::lookUp(scheme_).newReadHandle(*this);
}

std::string URI::asString() const {
    ASSERT(!name_.empty());
    ASSERT(!scheme_.empty());
    return URIManager::lookUp(scheme_).asString(*this);
}

std::string URI::asRawString() const {
    return scheme_ + ":" + name_;
}

void URI::print(std::ostream& s) const {
    s << "URI[scheme=" << scheme_ << ",name=" << name_ << "]";
}

void URI::encode(Stream &s) const {
    s << scheme_;
    s << name_;
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace eckit
