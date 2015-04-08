import System.Directory
import System.FilePath
import Data.Tree
import Test.QuickCheck
import Control.Applicative

gen :: Gen (Tree String)
gen = Node <$> listOf1 (elements ['a'..'z']) <*> listOf gen

main = do
  t <- generate gen
  cwd <- getCurrentDirectory
  go 0 cwd t
  where
  go n d t | n < 5 = do
    createDirectoryIfMissing False (d </> rootLabel t)
    mapM_ (go (n + 1) (d </> rootLabel t)) (subForest t)
           | otherwise = writeFile (d </> "placeholder.js") "// put code here" 
