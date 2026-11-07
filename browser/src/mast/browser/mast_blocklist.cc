// Copyright 2026 The Mast Authors

#include "mast/browser/mast_blocklist.h"

#include <optional>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "mast/browser/mast_feeds.h"
#include "url/gurl.h"

namespace mast {

namespace {

// blocklist/blocklist.json, embedded by remote/embed-pages.py. Edit the JSON, not this.
constexpr char kBlocklistJson[] =
#include "mast/browser/resources/blocklist_json.inc"
    ;

std::vector<std::string> Strings(const base::ListValue* list) {
  std::vector<std::string> out;
  if (list) {
    for (const base::Value& v : *list) {
      if (v.is_string()) {
        out.push_back(v.GetString());
      }
    }
  }
  return out;
}

std::vector<Category> Parse() {
  std::optional<base::Value> root = base::JSONReader::Read(kBlocklistJson, base::JSON_PARSE_RFC);
  CHECK(root && root->is_dict()) << "blocklist.json is not valid JSON";
  std::vector<Category> categories;
  const base::ListValue* list = root->GetDict().FindList("categories");
  CHECK(list);
  for (const base::Value& c : *list) {
    const base::DictValue& dict = c.GetDict();
    Category category;
    category.id = *dict.FindString("id");
    category.name = *dict.FindString("name");
    if (const base::ListValue* services = dict.FindList("services")) {
      for (const base::Value& s : *services) {
        const base::DictValue& sd = s.GetDict();
        category.services.push_back({*sd.FindString("name"),
                                     Strings(sd.FindList("domains")),
                                     Strings(sd.FindList("except"))});
      }
    }
    // A category backed by a feed gets one more row for everything else the
    // feed lists, e.g. "Other gambling sites" (mast_feeds.h).
    if (const std::string* feed = dict.FindString("feed")) {
      category.services.push_back(
          {"Other " + *feed + " sites", {FeedKey(*feed)}, {}});
    }
    categories.push_back(std::move(category));
  }
  return categories;
}

}  // namespace

const std::vector<Category>& Categories() {
  static const base::NoDestructor<std::vector<Category>> kCategories(Parse());
  return *kCategories;
}

bool IsExcepted(std::string_view host) {
  for (const Category& c : Categories()) {
    for (const Service& s : c.services) {
      for (const std::string& e : s.except) {
        if (HostMatches(host, e)) {
          return true;
        }
      }
    }
  }
  return false;
}

std::string CategoryNameOf(std::string_view domain) {
  for (const Category& c : Categories()) {
    for (const Service& s : c.services) {
      for (const std::string& d : s.domains) {
        if (d == domain) {
          return c.name;
        }
      }
    }
  }
  return std::string();
}

bool IsListed(std::string_view domain) {
  return !CategoryNameOf(domain).empty();
}

std::string NormalizeHost(std::string_view host) {
  std::string h = base::ToLowerASCII(host);
  for (std::string_view prefix : {"www.", "m.", "mobile."}) {
    if (h.starts_with(prefix)) {
      return h.substr(prefix.size());
    }
  }
  return h;
}

std::string CleanDomain(std::string_view input) {
  std::string text(base::TrimWhitespaceASCII(input, base::TRIM_ALL));
  if (text.empty()) {
    return std::string();
  }
  if (text.find("://") == std::string::npos) {
    text = "https://" + text;
  }
  GURL url(text);
  if (!url.is_valid() || !url.has_host() || url.host().find('.') == std::string::npos) {
    return std::string();
  }
  return NormalizeHost(url.host());
}

bool HostMatches(std::string_view host, std::string_view domain) {
  if (host == domain) {
    return true;
  }
  return host.size() > domain.size() && host.ends_with(domain) &&
         host[host.size() - domain.size() - 1] == '.';
}

}  // namespace mast
