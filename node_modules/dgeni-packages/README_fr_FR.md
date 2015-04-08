# Dgeni Packages

Ce dépôt contient une collection de **Packages** de Dgeni qui peuvent être utilisés par Dgeni, le générateur
de documentation, pour créer une documentation à partir du code source.


Actuellement, il y a les packages suivants :

* base - L'ensemble minimal des processeurs pour commencer avec Dgeni
* jsdoc - Extraction et analyse des balises
* nunjucks - Le moteur de rendu de template de nunjucks. Comme il n'est plus dans jsdoc, vous devez l'ajouter
  explicitement dans votre configuration ou vous obtiendrez l'erreur
  `Error: No provider for "templateEngine"! (Resolving: templateEngine)`
* ngdoc - La partie spécifique d'angular.js, avec la définition des balises, des processeurs et des templates.
  Celui-ci charge les packages de jsdoc et nunjucks pour vous.
* examples - Processeurs pour supporter les exemples exécutables qui figurent sur les docs du site d'angular.js.
* dgeni - support pour la documentation des packages de Dgeni (**incomplet**)

## Le package `base`

### Processeurs

* `computeIdsProcessor` - Détermine le `id` et le `aliases` pour les documents en utilisant des templates ou des fonctions
d'aide, sur la base du `docType`.
* `computePathsProcessor` - Détermine le `path` et le `outputPath` des documents en utilisant des templates ou des fonctions
d'aide, sur la base du `docType`.
* `debugDumpProcessor` - extrait l'état courant du tableau docs dans un fichier (désactivé par défaut)
* `readFilesProcessor` - utilisé pour charger les documents depuis les fichiers. Ce processeur peut-être configuré pour utiliser
un ensemble de **lecteur de fichier**. Il y a des lecteurs de fichiers (file-readers) dans les packages `jsdoc` et `ngdoc`.
* `renderDocsProcessor` - rendre les documents dans une propriété (`doc.renderedContent`) en utilisant
un `templateEngine` (moteur de template), qui doit être fourni séparément - voir le package `nunjucks`.
* `unescapeCommentsProcessor` - reformatte les marqueurs de commentaires pour ne pas casser le style des commentaires de jsdoc,
par exemple `*/`
* `writeFilesProcessor` - écrit les docs (pour ceux ont un `outputPath`) sur le disque

### Services

* `aliasMap` - Un map de ids/aliases pour les docs. C'est utilisé pour faire correspondre les références aux documents dans
des liens et des relations tels que les modules et les membres de l'objet.
* `createDocMessage` - une aide pour créer de beaux messages à prpopos des documents (utile pour les logs et
les erreurs)
* `encodeDocBlock` - convertir un bloc de code en HTML
* `templateFinder` - recherche dans les répertoires à l'aide de modèle (pattern) pour trouver un template qui correspond au document donné.
* `trimIndentation` - coupe "intelligemment" l'indentation dès le début de chaque ligne d'un bloc
de texte.
* `writeFile` - Ecrit du contenu dans un fichier, en s'assurant que le chemin du fichier existe.


#### Recherche du Template

Le template utilisé pour rendre un doc est déterminer par `templateFinder`, celui-ci utilise le premier qui correspond
à un ensemble de patterns dans un ensemble de dossiers, fourni dans la configuration. Cela permet pas mal de contrôle pour fournir
des templates génériques pour la plupart des situations et des templates spécifiques pour des cas exceptionnels.

Voici un exemple de plusieurs patterns de template standard:

```js
templateFinder.templatePatterns = [
  '${ doc.template }',
  '${doc.area}/${ doc.id }.${ doc.docType }.template.html',
  '${doc.area}/${ doc.id }.template.html',
  '${doc.area}/${ doc.docType }.template.html',
  '${ doc.id }.${ doc.docType }.template.html',
  '${ doc.id }.template.html',
  '${ doc.docType }.template.html'
]
```


## Le package `nunjucks`

Ce package fournit une implémentation de `templateEngine` basé sur nunjucks, qui est requis par le
`renderDocsPocessor` du package `base`. La boite à outils de template Javascript de "nunjucks" génére du HTML
basé sur les données de chaque document. Nous avons les templates, les tags et les filtres de nunjucks qui
peuvent rendre des liens et du texte en markdown et mettre le code en évidence.

### Services

* `nunjucks-template-engine` - fournit un `templateEngine` qui utilise la bibliothèque de template de Nunjucks
pour rendre les documents en texte, tel que le HTML ou le JS, basé sur des templates.

## Le package `jsdoc`

### Les lecteurs de fichier

* `jsdoc` - peut lire les documentations depuis les commentaires (avec le style jsdoc) dans les fichiers contenant le code source.

### Processeurs

* `codeNameProcessor` - détermine le nom du document selon le code qui suit dans le document
dans le fichier source.
* `extractTagsProcessor` - utilise un `tagExtractor` pour extraire l'information depuis les balises analysées.
* `inlineTagsProcessor` - recherche les docs pour les balises [`inline`](http://usejsdoc.org/about-inline-tags.html) qui ont besoin d'avoir de l'injection de contenu
* `parseTagsProcessor` - utilise un `tagParser` pour analyser les balises de jsdoc dans le contenu du document.

### Définitions des balises

Le package `jsdoc` contient des définitions pour un certain nombre de balise standard de jsdoc : `name`,
`memberof`, `param`, `property`, `returns`, `module`, `description`, `usage`,
`animations`, `constructor`, `class`, `classdesc`, `global`, `namespace`, `method`, `type` et
`kind`.

### Services (Transformations de balise)

Ce package fournit un certain nombre de services **Transform** qui sont utilisés dans les **définitions de balise** pour transformer
la valeur de la balise depuis le string du commentaire en quelque chose de plus significatif dans le doc.

* `extractNameTransform` - extrait un nom à partir d'une balise
* `extractTypeTransform` - extrait un type à partir d'une balise
* `trimWhitespaceTransform` - enlève les espaces avant et après la valeur de la balise
* `unknownTagTransform` - ajouter une erreur à la balise si elle est inconnue
* `wholeTagTransform` -  Utilise la balise comme valeur plutôt que d'utiliser une propriété de la balise

### Templates

**Ce package ne fournit pas de templates, ni un `templateEngine` pour rendre les templates (utilisez le
package `nunjucks` pour faire cela).**

### Définitions des balises

Ce package fournit une implémentation minimale des balises du projet JSDoc. Elles extraient le nom et
le type depuis la description de la balise trouvée, mais elles n'implémentent pas la totalité des fonctionnalités des balises JSDoc.

## Le package `ngdoc`

Le package `ngdoc` dépend des packages `jsdoc` et `nunjucks`. Il offre un support complémentaire pour
les documents non-API écrits dans les fichiers avec l'extension `.ngdoc`. Il détermine également des propriétés
supplémentaires spécifiques au code correspondant à Angular.

### Les lecteurs de fichier

* `ngdoc` - peut extraire un document depuis un fichier qui contient du ngdoc.

### Processeurs

* `filterNgdocsProcessor` -
Pour AngularJS, nous sommes seulement intéressés aux documents qui contiennent les balises @ngdoc. Ce processeur
supprime les docs qui ne contiennent pas cette balise.

* `generateComponentGroupsProcessor` -
Génère des documents pour chaque groupe de composants (par type) dans un module

* `memberDocsProcessor` - Ce processeur relie les docs qui sont membres (propriétés, méthodes et événements) à
leurs docs contenants, et les retire de la collection des docs principaux.

* `moduleDocsProcessor` - Ce processeur détermine les propriétés pour les docs des modules tels que `packageName` et
`packageFileName`. Il ajoute les modules au service `moduleMap` et relie tous les docs qui sont dans un module
au doc du moduledans la propriété `components`

* `providerDocsProcessor` - Ce processeur lie les documents sur les services angular au document de leur
provider correspondant.


### Définitions des balises

Ce package modifie et ajoute de nouvelles définitions de balises en plus de celles fournies par le package `jsdoc` :
`area`, `element`, `eventType`, `example`, `fullName`, `id`, `module`, `name`, `ngdoc`, `packageName`,
`parent`, `priority`, `restrict`, `scope` et `title`.


### Définitions des balises Inline

* `link` - Traite les balises Inline de lien (sous la forme {@link une/uri Un Titre}), en les remplaçant par
des ancres HTML


### Services

* `getAliases()` - Récupère une liste de tous les alias qui peuvent être faits à partir de la doc fournie
* `getDocFromAliases()` - Trouve un document depuis `aliasMap` qui correspond à l'alias donné
* `getLinkInfo()` - Récupère les informations du lien depuis un document qui correspond à l'url donné
* `gettypeClass()` - Récupère un string de classe CSS pour un string de type donné
* `moduleMap` - Une collection de modules correspondant à l'id du module


### Templates

Ce package fournit également un ensemble de templates pour générer un fichier HTML pour chaque document : api,
directive, erreur, fonction de filtre, input, module, objet, aperçu, provider, service, type et un numéro
pour supporter le rendu des exemples exécutables.

Vous devez être conscient qu'en raison de la superposition dans la syntaxe entre la liaison de données de Nunjucks et celle d'AngularJS,
le package ngdoc change les balises de liaisons de données par défaut de Nunjucks :

```js
templateEngine.config.tags = {
  variableStart: '{$',
  variableEnd: '$}'
};
```

### Filtres de rendu

* `code` - Rend un span de text comme du code
* `link` - Rend un lien HTML
* `typeClass` - Rend une classe CSS pour un type donné

### Balises de rendu

* `code` - Rend un bloc de code


## Le package `examples`

Ce package est un mix qui fournit des fonctionnalités pour travailler avec des exemples dans les docs.

À l'intérieur de vos documents, vous pouvez baliser example de la manière suivante :

```
Du texte avant l'exemple

<example name="example-name">
  <file name="index.html">
    <div>Le HTML principal de l'exemple</div>
  </file>
  <file name="app.js">
    // Du code JavaScript à inclure dans l'exemple
  </file>
</example>

Du texte après l'exemple
```


### Processeurs

* `generateExamplesProcessor` - Ajoute les nouveaux docs à la collection de docs pour chaque exemple dans le service `examples` qui sera rendue
comme des fichiers qui peuvent être exécutés dans le navigateur, par exemple des démos en direct ou pour
des tests de e2e. Ce processeur doit être configuré avec une collection des différents déploiements qui lui indiquera
la version à générer pour chaque exemple . Voir la section de **Configuration de déploiement** ci-dessous.
* `parseExamplesProcessor` - Analyse les balises `<example>` depuis le contenu et les ajoute au service `examples`
* `generateProtractorTestsProcessor` - Génère les fichiers de test de protractor depuis les tests e2e dans les exemples. Ce processeur
doit être configuré avec une collection des différents déploiements qui lui indiquera la version des tests de protaractor à générer. Voir la
section de **Configuration de déploiement** ci-dessous.

#### Configuration de déploiement

Les processeurs `generateExamplesProcessor` et `generateProtractorTestsProcessor` ont une propriété *obligatoire* appelée `deployments`.
Cette propriété doit être un tableau d'objets d'information de déploiement indiquant au processeur quels sont les fichiers à générer.

Par exemple, vous pourriez avoir un déploiement "debug" qui charge angular.js dans l'exemple et un déploiement "default" qui
charge angular.min.js dans l'exemple. De même, vous pourriez avoir des déploiements qui utilisent JQuery et certains qui utilisent
uniquement jqLite de Angular.

Vous pouvez configurer cela dans votre package comme ceci :

```js
.config(function(generateExamplesProcessor, generateProtractorTestsProcessor) {
  var deployments = [
    { name: 'debug', ... },
    { name: 'default', ... }
  ];

  generateExamplesProcessor.deployments = deployments;
  generateProtractorTestsProcessor.deployments = deployments;
});
```

Un déploiement doit avoir une propriété `name` et peut également inclure une propriété `examples` qui contient
des informations à propos du chemin et des fichiers supplémentaires à injecter dans des exemples.
De plus, un test de protractor est généré pour chaque déploiement et il utilise le nom du déploiement pour trouver le
chemin de l'exemple associé à ce déploiement.

```js
{
  name: 'default',
  examples: {
    commonFiles: {
      scripts: [ '../../../angular.js' ]
    },
    dependencyPath: '../../../'
  }
}
```

Ici nous avons un déploiement `default` qui injecte le fichier `angular.js` dans tous les exemples,
ainsi que les dépendances référencées dans l'exemple qui sont placées par rapport à la donnée `dependencyPath`.

### Définitions des balises Inline

* `runnableExample` -  Injecte l'exemple exécutable spécifié dans la doc


### Services

* `exampleMap` - une map contenant chaque exemple par un id. Cet id est généré de façon unique à partir
du nom de l'exemple

