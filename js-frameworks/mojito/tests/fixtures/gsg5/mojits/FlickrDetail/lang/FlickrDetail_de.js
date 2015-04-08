/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add("lang/FlickrDetail_de", function(Y) {

    Y.Intl.add(

        "FlickrDetail", // associated module
        "de",           // BCP 47 language tag

        // key-value pairs for this module and language
        {
            INFO_NO_IMAGE_CHOSEN: "Bild nicht gewählt",
            ERROR_BAD_IMAGE_ID: "Fehler! schlechtes Image-Kennung.",
            ERROR_NO_DETAILS: "Wir konnten zu Informationen für Foto abzurufen.",
            DATE_POSTED: "Erstellungsdatum",
            TITLE: "Titel",
            TITLE_NONE: "kein",
            DESCRIPTION: "Beschreibung",
            DESCRIPTION_NONE: "keine",
            OWNER_USERNAME: "Benutzername",
            TAGS: "Begriffe",
            TAGS_NONE: "keine",
            URLS: "URLs",
            URL_PHOTO_PAGE: "Seite",
            URL_IMAGE: "Bild"
        }
    );
}, "3.1.0", {requires: ['intl']});
