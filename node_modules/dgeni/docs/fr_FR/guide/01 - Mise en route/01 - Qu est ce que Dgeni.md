# Qu'est ce que Dgeni ?

Dgeni est un générateur de documentation construit par l'équipe d'AngularJS. A l'origine, il a été créé pour
construire la documentation du projet AngularJS.

Le projet se compose d'un package Node.js, qui peut être exécuté directement comme un outil en ligne de commande ou
intégré facilement dans les outils de construction tels que Grunt ou Gulp.

## Objectifs du projet

L'objectif principal du projet Dgeni est d'aider les développeurs de logiciels pour la génération de
la documentation de leur code.

Pour atteindre cet objectif, le projet fournit un outil qui se veut indépendant de la technologie,
vous pouvez l'utiliser pour documenter tout type de projet, provenant d'applications AngularJS JavaScript
ou du code écrit en C.

L'outil doit fournir une solution préconfigurée simple pour les cas les plus courants, mais doit être entièrement
configurable et personnalisable pour les autres situations relatives à la documentation.


## Comment ça marche ?

La flexibilité de Dgeni est obtenue en déléguant la génération de la documentation à un canal
de "processeurs de document", qui sont définies dans des packages comme faisant partie de la
configuration de Dgeni.

Une fois configuré, Dgeni exécutera simplement chaque processeur les uns après les autres. Chaque processeur
reçoit une collection d'objets de document, qui est retournée par le processeur précédent. Le processeur
modifie alors cette collection ou les propriétés des objets de document dans la collection et renvoie la
collection pour le processeur suivant.

Chaque processeur doit avoir une seule tâche bien définie, ceci facilite la maintenance et la réutilisation
des processeurs.

Les processeurs les plus important auront l'une des tâches suivantes :

- le chargement du document source depuis les fichiers
- l'analyse de la source du document pour les méta-données (comme les balises de jsdoc)
- la détermination des propriétés supplémentaires sur la base de celles qui ont été analysés, comme
  le nom du document
- la fabrication des documents sous une forme lisible, tels que le HTML, généralement aidé par des templates
- la rédaction des documents rendus dans des fichiers

## Dgeni et AngularJS

Le projet AngularJS, utilise son propre type d'annotation quand il s'agit de documenter son code,
connu sous le nom des balises "ngdoc". Ces balises sont similaires mais pas officiellement pris en charge
par les générateurs de documentation comme JSDoc. Par conséquent, l'équipe d'AngularJS a construit son propre
générateur de documentation qui fonctionne comme jsdoc mais a été codée en dur pour fonctionner spécifiquement
avec les balises de ngdoc et le type particulier de la structure qu'AngularJS utilise dans son code.

Cependant, il s'est avéré que l'implémentation du générateur NGDoc était difficile à maintenir et à changer,
car les templates ne sont pas séparés de la base du code actuel. De plus, l'ajout de nouvelles fonctionnalités
n'était pas facile pour les nouveaux contributeurs. En plus, le générateur entier a été construit dans le code
d'AngularJS, ce qui rend sa réutilisation difficile pour d'autres équipes de développement qui veulent documenter
leur propre applications AngularJS et leurs bibliothèques.

AngularJS a résolu ce problème en écrivant et en utilisant Dgeni. Il fournit maintenant un ensemble de packages qui
prennent en charge la documentation d'AngularJS. Comme ceux-ci sont modulaires et séparés de la base du code d'AngularJS,
il est plus facile de maintenir les processeurs et il est aussi possible pour les autres équipes de les réutiliser.

## Dois-je utiliser Dgeni ?

Dgeni peut être utilisé pour documenter tout type de code, à la fois côté client, comme des applications
AngularJS, et côté serveur, comme un serveur RESTful Node.js.

Il est plutôt agnostique lorsqu'il s'agit de la technologie utilisée dans votre projet. Comme vous
pouvez créer vos propres packages personnalisés, vous pouvez l'utiliser pour documenter quoi que ce soit depuis
une application .NET de Windows ou depuis un serveur PHP. Il a juste besoin de vous ou de quelqu'un d'autre qui
est disposé à écrire les processeurs de document nécessaires.

Cela étant dit, Dgeni est écrit en Node.js par l'équipe d'AngularJS, donc il est probable qu'il y aura
plus de processeurs de document qui seront fournis pour supporter la documentation d'un projet JavaScript.
