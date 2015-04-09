(:
 : JSONiq Query
 :)
for $answers in collection("answers")
group by $user-id := $answers.owner.user_id
let $count := count($answers)
let $avg-score := avg($answers.score)
order by $avg-score, $count descending
let $titles := for $answer in $answers
               for $question in collection("faq")
               where $answer.question_id eq $question.question_id
               return $question.title
return {
  name: $answers[1].owner.display_name,
  reputation: $avg-score,
  count: $count,
  titles: [distinct-values($titles)]
}
