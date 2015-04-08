/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add("lang/FlickrDetail_en-US", function (Y) {
    "use strict";
    Y.Intl.add(

        "FlickrDetail", // associated module
        "en-US",        // BCP 47 language tag

        // key-value pairs for this module and language
        {
            INFO_NO_IMAGE_CHOSEN: "No image chosen.",
            ERROR_BAD_IMAGE_ID: "Error! Bad image ID.",
            ERROR_NO_DETAILS: "Failed to retrieve details for photo.",
            DATE_POSTED: "posted",
            TITLE: "title",
            TITLE_NONE: "none",
            DESCRIPTION: "description",
            DESCRIPTION_NONE: "none",
            OWNER_USERNAME: "username",
            TAGS: "tags",
            TAGS_NONE: "none",
            URLS: "urls",
            URL_PHOTO_PAGE: "page",
            URL_IMAGE: "image"
        }
    );
}, "3.1.0", {requires: ['intl']});
