import {MultiTransformer} from 'traceur/src/codegeneration/MultiTransformer';
import {UniqueIdentifierGenerator} from 'traceur/src/codegeneration/UniqueIdentifierGenerator';
import {options} from 'traceur/src/Options';

import {ArrowFunctionTransformer} from './ArrowFunctionTransformer';
import {ClassTransformer} from './ClassTransformer';
import {InstanceOfTransformer} from './InstanceOfTransformer';
import {MultiVarTransformer} from './MultiVarTransformer';
import {StrictEqualityTransformer} from './StrictEqualityTransformer';
import {NamedParamsTransformer} from './NamedParamsTransformer';
import {ExportTransformer} from './ExportTransformer';
import {ForOfTransformer} from './ForOfTransformer';
import {DestructuringTransformer} from './DestructuringTransformer';

/**
 * Transforms ES6 + annotations to Dart code.
 */
export class DartTransformer extends MultiTransformer {
  constructor(reporter, options) {
    super(reporter, options.validate);
    let idGenerator = new UniqueIdentifierGenerator();

    var append = (transformer) => {
      this.append((tree) => {
        return new transformer(idGenerator, reporter).transformAny(tree);
      });
    };

    append(ArrowFunctionTransformer);
    append(NamedParamsTransformer);
    append(MultiVarTransformer);
    append(InstanceOfTransformer);
    append(StrictEqualityTransformer);
    append(ClassTransformer);
    append(ExportTransformer);
    append(ForOfTransformer);
    append(DestructuringTransformer);
  }
}
