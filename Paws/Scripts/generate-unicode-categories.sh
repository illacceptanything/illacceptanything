curl -O http://www.unicode.org/Public/7.0.0/ucd/extracted/DerivedGeneralCategory.txt

categories="$@"
if [ ! -n "$categories" ]; then
   categories="Cn Lu Ll Lt Lm Lo Mn Me Mc Nd Nl No Zs Zl Zp Cc Cf Co Cs Pd Ps Pe Pc Po Sm Sc Sk So Pi Pf"
fi

for category in $categories; do
   line="$category"' = ['$(
      grep "; $category" DerivedGeneralCategory.txt |
         cut -f1 -d " " |
         grep -v '[0-9a-fA-F]\{5\}' |
         sed -e 's/\.\./-/' |
         sed -e 's/\([0-9a-fA-F]\{4\}\)/\\u\1/g' |
         tr -d '\n'
   )']'
   
   echo
   echo "$line"
done

rm -i DerivedGeneralCategory.txt
