# Pourquoi Dgeni ?

Maintenant, pourquoi veut-on utiliser Dgeni comme générateur de documentation ? Il existe déjà beaucoup d'autres
outils ? On pourrait simplement utiliser [JSDoc](http://usejsdoc.org/) ou YUIDoc(http://yui.github.io/yuidoc/),
les deux font du très bon travail. C'est exact. Cependant, il y a plusieurs raisons pour lesquelles on veut
probablement utiliser Dgeni à la place.

- **Votre code a la syntaxe AngularJS**

  Vous avez toujours voulu annoter votre code avec des annotations appropriées pour les services, les filtres ou
  les directives. Comment le faire sur les modules et les contrôleurs ? Pouvez-vous annoter votre code avec des outils
  comme JSDoc ou YUIDoc ? Ces générateurs de documents ne supportent pas la syntaxe spécifique d'AngularJS. Cependant
  Dgeni peut être étendue avec un package NGDoc qui vous donne exactement ces fonctionnalités. En d'autres termes,
  si vous travaillez sur du code AngularJS et que vous souhaitez le documenter, Dgeni est probablement le seul
  outil qui rend cela possible.

- **Vous voulez une annotation personnalisée**

  De la même manière qu'AngularJS apporte son propre sucre syntaxique pour des types spécifiques de
  composants, vous voulez probablement avoir vos propres types d'annotations. Cela s'explique par différentes
  raisons. Par exemple, imaginez le cas où vous travaillez sur une bibliothèque ou un framework qui doit être
  capable de traiter les annotations spécifiques d'AngularJS ainsi que des annotations VanillaJS. Dans ce cas,
  vous ne voulez probablement pas répandre partout les annotations `@ngdoc`, mais plutôt avoir vos propres
  balises d'annotation personnalisées qui déclare votre espace de noms spécifique.

- **Flexibilité**

  Dgeni est plus ou moins un tuyau qui exécute une série de différents processeurs pour traiter les données
  relatives aux documents proposés. Maintenant, qu'est ce cela a à voir avec la flexibilité ? Le fait d'être en mesure
  d'ajouter, enlèver ou changer l'ordre des processeurs, vous donne le contrôle total sur la façon dont votre
  documentation est traitée et générée. **Vous** pouvez choisir les données de la documentation qui sont pertinentes
  pour vous. **Vous** pouvez choisir les fonctionnalités qui seront activé dans votre documentation généré.
  **Vous** pouvez décider comment votre documentation sera rendue.

- **Modularité**

  Cela va de pair avec la **flexibilité**. Mais le véritable avantage d'être modulaire, est le fait qu'il
  est incroyablement facile d'ajouter des fonctionnalités supplémentaires. Des fonctionnalité qui n'existe
  même pas encore. Si vous le souhaitez, vous pouvez créer vos propres packages personnalisés et apporter des
  fonctionnalités que personne d'autre a dans sa documentation. Bien sûr, vous pouvez aussi supprimer une
  fonctionnalité qui ne correspond pas à vos besoins.

Ce ne sont que quelques arguments qui peuvent vous convaincre d'utiliser Dgeni comme générateur de documentation.
Évaluez le par vous-même si cela vous semble être le bon outil.
