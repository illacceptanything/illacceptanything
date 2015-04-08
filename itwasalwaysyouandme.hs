data Who = Me | You
instance Eq Who where
	(==) Me Me = True
	(==) You You = True
	(==) _ _ = False
