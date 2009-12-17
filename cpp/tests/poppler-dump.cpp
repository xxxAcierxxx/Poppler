/*
 * Copyright (C) 2009, Pino Toscano <pino@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <poppler-document.h>
#include <poppler-embedded-file.h>
#include <poppler-font.h>
#include <poppler-page.h>
#include <poppler-toc.h>

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

#include "parseargs.h"

static const int out_width = 30;

bool show_info = false;
bool show_perm = false;
bool show_metadata = false;
bool show_toc = false;
bool show_fonts = false;
bool show_embedded_files = false;
bool show_pages = false;
bool show_help = false;

static const ArgDesc the_args[] = {
    { "--show-info",           argFlag,  &show_info,           0,
      "show general document information" },
    { "--show-perm",           argFlag,  &show_perm,           0,
      "show document permissions" },
    { "--show-metadata",       argFlag,  &show_metadata,       0,
      "show document metadata" },
    { "--show-toc",            argFlag,  &show_toc,            0,
      "show the TOC" },
    { "--show-fonts",          argFlag,  &show_fonts,          0,
      "show the document fonts" },
    { "--show-embedded-files", argFlag,  &show_embedded_files, 0,
      "show the document-level embedded files" },
    { "--show-pages",          argFlag,  &show_pages,          0,
      "show pages information" },
    { "-h",                    argFlag,  &show_help,           0,
      "print usage information" },
    { "--help",                argFlag,  &show_help,           0,
      "print usage information" },
    { NULL, argFlag, 0, 0, NULL }
};

static void error(const std::string &msg)
{
    std::cerr << "Error: " << msg << std::endl;
    std::cerr << "Exiting..." << std::endl;
    exit(1);
}

static std::string out_ustring(const poppler::ustring &str)
{
    return str.to_latin1();
}

static std::string out_date(std::time_t date)
{
    if (date != std::time_t(-1)) {
        struct tm time;
        gmtime_r(&date, &time);
        char buf[32];
        strftime(buf, sizeof(buf) - 1, "%d/%m/%Y %H:%M:%S", &time);
        return std::string(buf);
    }
    return std::string("n/a");
}

static std::string out_size(int size)
{
    if (size >= 0) {
        std::ostringstream ss;
        ss << size;
        return ss.str();
    }
    return std::string("n/a");
}

static char charToHex(int x)
{
    return x < 10 ? x + '0' : x - 10 + 'a';
}

static std::string out_hex_string(const std::string &str)
{
    std::string ret(str.size() * 2, '\0');
    const char *str_p = str.data();
    for (unsigned int i = 0; i < str.size(); ++i, ++str_p) {
        ret[i * 2] = charToHex((*str_p & 0xf0) >> 4);
        ret[i * 2 + 1] = charToHex(*str_p & 0xf);
    }
    return ret;
}

static void print_info(poppler::document *doc)
{
    std::cout << "Document information:" << std::endl;
    int major = 0, minor = 0;
    doc->get_pdf_version(&major, &minor);
    std::cout << std::setw(out_width) << "PDF version" << ": " << major << "." << minor << std::endl;
    const std::vector<std::string> keys = doc->info_keys();
    std::vector<std::string>::const_iterator key_it = keys.begin(), key_end = keys.end();
    for (; key_it != key_end; ++key_it) {
        std::cout << std::setw(out_width) << *key_it << ": " << out_ustring(doc->info_key(*key_it)) << std::endl;
    }
    std::cout << std::setw(out_width) << "Date (creation)" << ": " << out_date(doc->info_date("CreationDate")) << std::endl;
    std::cout << std::setw(out_width) << "Date (modification)" << ": " << out_date(doc->info_date("ModDate")) << std::endl;
    std::cout << std::setw(out_width) << "Number of pages" << ": " << doc->pages() << std::endl;
    std::cout << std::setw(out_width) << "Linearized" << ": " << doc->is_linearized() << std::endl;
    std::cout << std::setw(out_width) << "Encrypted" << ": " << doc->is_encrypted() << std::endl;
    std::cout << std::endl;
}

static void print_perm(poppler::document *doc)
{
    std::cout << "Document permissions:" << std::endl;
#define OUT_PERM(theperm) \
    std::cout << std::setw(out_width) << "Perm (" #theperm ")" << ": " \
              << doc->has_permission(poppler::perm_##theperm) << std::endl
    OUT_PERM(print);
    OUT_PERM(change);
    OUT_PERM(copy);
    OUT_PERM(add_notes);
    OUT_PERM(fill_forms);
    OUT_PERM(accessibility);
    OUT_PERM(assemble);
    OUT_PERM(print_high_resolution);
    std::cout << std::endl;
#undef OUT_PERM
}

static void print_metadata(poppler::document *doc)
{
    std::cout << std::setw(out_width) << "Metadata" << ":" << std::endl
              << out_ustring(doc->metadata()) << std::endl;
    std::cout << std::endl;
}

static void print_toc_item(poppler::toc_item *item, int indent)
{
    std::cout << std::setw(indent * 2) << " "
              << "+ " << out_ustring(item->title()) << " (" << item->is_open() << ")"
              << std::endl;
    poppler::toc_item::iterator it = item->children_begin(), it_end = item->children_end();
    for (; it != it_end; ++it) {
        print_toc_item(*it, indent + 1);
    }
}

static void print_toc(poppler::toc *doctoc)
{
    std::cout << "Document TOC:" << std::endl;
    if (doctoc) {
        print_toc_item(doctoc->root(), 0);
    } else {
        std::cout << "<no TOC>" << std::endl;
    }
    std::cout << std::endl;
}

static void print_fonts(poppler::document *doc)
{
    std::cout << "Document fonts:" << std::endl;
    std::vector<poppler::font_info> fl = doc->fonts();
    if (!fl.empty()) {
        std::vector<poppler::font_info>::const_iterator it = fl.begin(), it_end = fl.end();
        const std::ios_base::fmtflags f = std::cout.flags();
        std::left(std::cout);
        for (; it != it_end; ++it) {
            std::cout
                << " " << std::setw(out_width + 10) << it->name()
                << " " << std::setw(15) << it->type()
                << " " << std::setw(5) << it->is_embedded()
                << " " << std::setw(5) << it->is_subset()
                << " " << it->file()
                << std::endl;
        }
        std::cout.flags(f);
    } else {
        std::cout << "<no fonts>" << std::endl;
    }
    std::cout << std::endl;
}

static void print_embedded_files(poppler::document *doc)
{
    std::cout << "Document embedded files:" << std::endl;
    std::vector<poppler::embedded_file *> ef = doc->embedded_files();
    if (!ef.empty()) {
        std::vector<poppler::embedded_file *>::const_iterator it = ef.begin(), it_end = ef.end();
        const std::ios_base::fmtflags f = std::cout.flags();
        std::left(std::cout);
        for (; it != it_end; ++it) {
            poppler::embedded_file *f = *it;
            std::cout
                << " " << std::setw(out_width + 10) << f->name()
                << " " << std::setw(10) << out_size(f->size())
                << " " << std::setw(20) << out_date(f->creation_date())
                << " " << std::setw(20) << out_date(f->modification_date())
                << std::endl
                << "     " << (f->description().empty() ? std::string("<no description>") : out_ustring(f->description()))
                << std::endl
                << "     " << std::setw(35) << (f->checksum().empty() ? std::string("<no checksum>") : out_hex_string(f->checksum()))
                << " " << (f->mime_type().empty() ? std::string("<no mime type>") : f->mime_type())
                << std::endl;
        }
        std::cout.flags(f);
    } else {
        std::cout << "<no embedded files>" << std::endl;
    }
    std::cout << std::endl;
}

static void print_page(poppler::page *p)
{
    std::cout << std::setw(out_width) << "Rect" << ": " << p->page_rect() << std::endl;
    std::cout << std::setw(out_width) << "Label" << ": " << out_ustring(p->label()) << std::endl;
    std::cout << std::setw(out_width) << "Duration" << ": " << p->duration() << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    if (!parseArgs(the_args, &argc, argv)
        || argc < 2 || show_help) {
        printUsage(argv[0], "DOCUMENT", the_args);
        exit(1);
    }

    std::string file_name(argv[1]);

    std::auto_ptr<poppler::document> doc(poppler::document::load_from_file(file_name));
    if (!doc.get()) {
        error("loading error");
    }
    if (doc->is_locked()) {
        error("encrypted document");
    }

    std::cout.setf(std::ios_base::boolalpha);

    if (show_info) {
        print_info(doc.get());
    }
    if (show_perm) {
        print_perm(doc.get());
    }
    if (show_metadata) {
        print_metadata(doc.get());
    }
    if (show_toc) {
        std::auto_ptr<poppler::toc> doctoc(doc->create_toc());
        print_toc(doctoc.get());
    }
    if (show_fonts) {
        print_fonts(doc.get());
    }
    if (show_embedded_files) {
        print_embedded_files(doc.get());
    }
    if (show_pages) {
        const int pages = doc->pages();
        for (int i = 0; i < pages; ++i) {
            std::cout << "Page " << (i + 1) << "/" << pages << ":" << std::endl;
            std::auto_ptr<poppler::page> p(doc->create_page(i));
            print_page(p.get());
        }
    }

    return 0;
}
