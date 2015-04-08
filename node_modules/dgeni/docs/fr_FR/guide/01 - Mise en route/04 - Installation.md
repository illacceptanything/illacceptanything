# Installation

Ce document vous guide à travers le processus d'installation de Dgeni sur différentes plateformes.

## Prérequis

Pour installer Dgeni sur votre machine en local, plusieurs outils doivent être d'abord présents.

- **Node.js**


  Vous aurez besoin d'installer [Node.js](http://nodejs.org). Il suffit de télécharger le fichier binaire sur le
  site Web et l'installer sur votre machine.

- **npm**

  *npm* est le "Node Package Manager". Il est généralement fourni avec le binaire de Node.js.

Vous pouvez également installer ces outils manuellement en jetant un oeil à ce [gist](https://gist.github.com/isaacs/579814).

## Installation de Dgeni via npm

L'installation de Dgeni est très facile, il suffit d'exécuter la commande suivante dans votre terminal (en
supposant que vous vous trouvez dans le dossier de votre projet) :

```js
npm install dgeni
```

Pour ajouter Dgeni  à votre projet comme une dépendance de développement, exécuter cette commande avec l'option `--save-dev`.
Cela ajoutera Dgeni à la liste des `devDependencies` dans votre fichier `package.json`.

```js
npm install --save-dev dgeni
```

Ces deux commandes vont installer Dgeni dans votre projet sous le dossier `node_modules/`. Dgeni est maintenant
installé localement et est prêt à l'emploi.
